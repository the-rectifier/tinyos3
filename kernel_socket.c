#include "tinyos.h"
#include "kernel_socket.h"
#include "kernel_cc.h"


SCB * PORT_MAP[MAX_PORT + 1] = {NULL};


static file_ops socket_fops = {
	.Open = dummy_socket_open,
	.Read = socket_read,
	.Write = socket_write,
	.Close = socket_close
};


Fid_t sys_Socket(port_t port)
{
	/* validity of given port */
	if(port < 0 || port > MAX_PORT){
		return NOFILE;
	}
	/* Declare one fid and one fcb */
	Fid_t fid;
	FCB * fcb;

	/* Reserve them if possible */
	if(!FCB_reserve(1, &fid, &fcb)){
		return NOFILE;
	}

	/* create and init the socket */
	SCB * scb = init_SCB(fcb, port);
	
	/* associate the fcb and the scb */
	fcb->streamobj = scb;
	fcb->streamfunc = &socket_fops;

	return fid;
}


int sys_Listen(Fid_t sock)
{	
	/* Grab scb associated with fd */
	SCB * scb = get_scb(sock);

	/** return -1 
	 * if the port is invalid
	 * the port table is occupied
	 * SCB is NULL 
	 * SCB is not UNBOUND
	 */
	if(scb == NULL || scb->type != SOCKET_UNBOUND || scb->port < 1 || scb->port > MAX_PORT || PORT_MAP[scb->port] != NULL){
		return -1;
	}

	scb->type = SOCKET_LISTENER;
	/* bind the socket to the port */
	PORT_MAP[scb->port] = scb;
	
	

	/* allocate the proper union field */
	scb->props.listener_s = (lsock_t *)xmalloc(sizeof(lsock_t));

	/* init the condition variable and request list */
	scb->props.listener_s->req_available = COND_INIT;
	rlnode_init(&scb->props.listener_s->req_queue, NULL);

	return 0;
}


Fid_t sys_Accept(Fid_t lsock)
{
	SCB * server = get_scb(lsock);
	/* check server type */
	if(server == NULL || server->type != SOCKET_LISTENER){
		return NOFILE;
	}

	/* do not disturb */
	server->refcount++;

	/* sleep until a request is made */
	while(is_rlist_empty(&server->props.listener_s->req_queue)){
		kernel_wait(&server->props.listener_s->req_available, SCHED_PIPE);
	}

	/* get the request from the list */
	rlnode * request = rlist_pop_front(&server->props.listener_s->req_queue);
	
	/* since we woke up the request shouldn't be null */
	assert(request != NULL);

	/* get the request struct from the list node */
	request_t * request_s = request->request_s;

	/* check the server again */
	server = get_scb(lsock);
	/* create a new socket to serv the client */
	Fid_t peer = sys_Socket(NOPORT);
	/* get the serv socket */
	SCB * serv_client = get_scb(peer);
	/* get the client socket */
	SCB * client = request_s->peer;

	/* return error if the server is null or no socket was createt for the client */
	if(server == NULL || peer == NOFILE){
		kernel_signal(&request_s->connected_cv);
		return NOFILE;
	}

	/* mark current structs as PEERS */
	serv_client->type = SOCKET_PEER;
	client->type = SOCKET_PEER;

	/* allocate the union field for the established connection */
	serv_client->props.peer_s = (psock_t *)xmalloc(sizeof(psock_t));
	client->props.peer_s = (psock_t *)xmalloc(sizeof(psock_t));

	/* connect the 2 peers */
	serv_client->props.peer_s->peer = client;
	client->props.peer_s->peer = serv_client;

	/* 2 fcbs */
	FCB * fcbs[2];

	/* connect FCB to socket */
	fcbs[READ] = serv_client->fcb;
	fcbs[WRITE] = client->fcb;
	/* create a pipe without fds, the opposite read/write for client*/
	PIPE_CB * serv_pipe_read = init_PIPE_CB(fcbs);
	/* update the connections in the newly edited sockets */
	serv_client->props.peer_s->read_pipe = serv_pipe_read;
	client->props.peer_s->write_pipe = serv_pipe_read;

	/** redo the above just with reversed fcb positions
	 * client reads where server writes
	 * client writes where server reads
	 */
	fcbs[WRITE] = serv_client->fcb;
	fcbs[READ] = client->fcb;
	PIPE_CB * serv_pipe_write = init_PIPE_CB(fcbs);
	/* update the connections in the newly edited sockets */
	serv_client->props.peer_s->write_pipe = serv_pipe_write;
	client->props.peer_s->read_pipe = serv_pipe_write;
	

	/* update request indicator */
	request_s->admitted = 1;

	/* tell the client */
	kernel_signal(&request_s->connected_cv);

	/* you may enter */
	server->refcount--;

	return peer;
}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	SCB * scb = get_scb(sock);

	/** 
	 * return error if
	 * scb is NULL
	 * invalid port
	 * no socket on port
	 * invalid socket type on port 
	 */

	if(scb == NULL || port < 1 || port >= MAX_PORT || PORT_MAP[port] == NULL || scb->type != SOCKET_UNBOUND || PORT_MAP[port]->type != SOCKET_LISTENER){
		return -1;
	}

	/* do not disturb */
	scb->refcount++;

	SCB * lscb = PORT_MAP[port];

	/* craft the request */
	request_t * request_s = craft_request(scb);

	/* push the request back of the listeners queue */
	rlist_push_back(&lscb->props.listener_s->req_queue, &request_s->queue_node);

	/* tell the listener */
	kernel_signal(&lscb->props.listener_s->req_available);

	/* wait until timeout */
	kernel_timedwait(&lscb->props.listener_s->req_available, SCHED_PIPE, timeout);

	/* restore ref count */
	scb->refcount--;

	/* return 0 if connected -1 if failed */
	return (request_s->admitted) ? 0 : -1;
}


SCB * init_SCB(FCB * fcb, port_t port){
	/** 
	 * allocate SCB struct 
	 * initialize each field respectively
	 */
	SCB * scb = (SCB*)xmalloc(sizeof(SCB));
	scb->refcount = 1;
	scb->fcb = fcb;
	scb->port = port;
	scb->type = SOCKET_UNBOUND;
	return scb;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	SCB * scb = get_scb(sock);

	switch (how){
	case SHUTDOWN_READ:
		return pipe_read_close(scb->props.peer_s->read_pipe);
	case SHUTDOWN_WRITE:
		return pipe_write_close(scb->props.peer_s->write_pipe);
	case SHUTDOWN_BOTH:
		return socket_close(scb);
	}
	return -1;
}


int socket_read(void* socket_cb, char * buffer, unsigned int n){
	SCB * scb = (SCB *)socket_cb;
	/** 
	 * try to read only if the socket is connected 
	 * return -1 if not
	 */
	if(scb->type == SOCKET_PEER){
		PIPE_CB * pipe = scb->props.peer_s->read_pipe;
		return pipe_read(pipe, buffer, n);
	}
	return -1;
}


int socket_write(void* socket_cb, const char *buffer, unsigned int n){
	SCB * scb = (SCB *)socket_cb;
	/** 
	 * try to write only if the socket is connected 
	 * return -1 if not
	 */
	if(scb->type == SOCKET_PEER){
		PIPE_CB * pipe = scb->props.peer_s->write_pipe;
		return pipe_write(pipe, buffer, n);
	}
	return -1;
}


int socket_close(void * socket_cb){
	SCB * scb = (SCB *)socket_cb;
	if(scb == NULL){
		return -1;
	}

	switch(scb->type){
		case SOCKET_UNBOUND:
			break;
		case SOCKET_LISTENER:
			/* free the port number */
			PORT_MAP[scb->port] = NULL;
			/* free the list of requests and signal each client */
			while(!is_rlist_empty(&scb->props.listener_s->req_queue)){
				rlist_pop_back(&scb->props.listener_s->req_queue);
				kernel_signal(&scb->props.listener_s->req_available);
			}
			/* free the struct */
			free(scb->props.listener_s);
			break;
		case SOCKET_PEER:
			/* check socket if connected, close pipes when necessary and set the client-side peer to NULL*/
			if(scb->props.peer_s->peer != NULL){
				pipe_read_close(scb->props.peer_s->read_pipe);
				pipe_write_close(scb->props.peer_s->write_pipe);
				scb->props.peer_s->peer->props.peer_s->peer = NULL;
			}
			free(scb->props.peer_s);
			break;
	}

	free(scb);
	return 0;
}


void * dummy_socket_open(uint fd){
	return NULL;
}


SCB *get_scb(Fid_t fid){
	/* Get FCB */
	FCB * fcb = get_fcb(fid);
	/* return SCB or NULL */
	return (fcb != NULL) ? (SCB *)fcb->streamobj : NULL;
}


request_t * craft_request(SCB * scb){
	request_t * request_s = (request_t *)xmalloc(sizeof(request_t));
	request_s->admitted = 0;
	request_s->connected_cv = COND_INIT;
	request_s->peer = scb;
	rlnode_init(&request_s->queue_node, request_s);

	return request_s;
}

