#include "tinyos.h"
#include "kernel_socket.h"
#include "kernel_cc.h"

SCB * PORT_MAP[MAX_PORT] = {NULL};

file_ops socket_file_ops = {
        .Open = NULL,
        .Read = socket_read,
        .Write = socket_write,
        .Close = socket_close
};

SCB * init_scb(FCB * fcb, port_t port){
	SCB * socket = (SCB *) xmalloc(sizeof(SCB));
	socket->fcb = fcb;
	socket->port = port;
	socket->refcount = 0;
	socket->type = SOCKET_UNBOUND;

	return socket;
}

Fid_t sys_Socket(port_t port)
{
	if((port < 0 || port >= MAX_PORT))
		return NOFILE;

	Fid_t  fid;
	FCB * fcb;

	if(!FCB_reserve(1, &fid, &fcb))
		return NOFILE;

	SCB * socket_cb = init_scb(fcb, port);

	fcb->streamobj = socket_cb;
	fcb->streamfunc = &socket_file_ops;

	return fid;
}

int sys_Listen(Fid_t sock)
{
	FCB * fcb = get_fcb(sock);

	if(fcb == NULL)		//illegal fid -> fcb = null
		return -1;

	SCB * lsocket = fcb->streamobj;
	assert(lsocket != NULL);

	if(lsocket->type != SOCKET_UNBOUND || lsocket->port == NOPORT || PORT_MAP[lsocket->port] != NULL)			//socket is not bound to a port or port is occupied
		return -1;

	PORT_MAP[lsocket->port] = lsocket;	//add the listener scb to the PORT_MAP

	lsocket->type = SOCKET_LISTENER;		//change type from unbound to listener
	lsocket->socket_struct.listener_s = (Listener_socket *) xmalloc(sizeof(Listener_socket));
	lsocket->socket_struct.listener_s->req_available_cv = COND_INIT;
	rlnode_init(&lsocket->socket_struct.listener_s->request_queue, lsocket);  //initialize request_queue

	return 0;

}


Fid_t sys_Accept(Fid_t lsock)
{
	FCB * fcb = get_fcb(lsock);

	if(fcb == NULL)
		return NOFILE;

	SCB * lsocket_cb = fcb->streamobj;		//get the scb from the fcb
	assert(lsocket_cb != NULL);

	if(lsocket_cb->type != SOCKET_LISTENER || PORT_MAP[lsocket_cb->port] != lsocket_cb)
		return NOFILE;
	
	lsocket_cb->refcount++;					//increase refcount

	while(is_rlist_empty(&lsocket_cb->socket_struct.listener_s->request_queue))		//while list is empty wait 
		kernel_wait(&lsocket_cb->socket_struct.listener_s->req_available_cv, SCHED_PIPE);	//for a request

	if(lsocket_cb == NULL || PORT_MAP[lsocket_cb->port] != lsocket_cb)  //check if port is still available
		return NOFILE;

	rlnode * req_queue_node = rlist_pop_front(&lsocket_cb->socket_struct.listener_s->request_queue);
	Con_request * request = req_queue_node->request;
	
	request->admitted = 1;
	Fid_t new_peer_fid = sys_Socket(NOPORT);		//Create new peer socket_cb
	if(new_peer_fid == NOFILE)
		return NOFILE;

	SCB * peer_scb = request->peer_SCB;							//get the scb that made the request
	SCB * new_peer_scb = get_fcb(new_peer_fid)->streamobj;		//create new peer socket

	peer_scb->socket_struct.peer_s = (Peer_socket *) xmalloc(sizeof(Peer_socket));
	new_peer_scb->socket_struct.peer_s = (Peer_socket *) xmalloc(sizeof(Peer_socket));
	
	FCB * fcb1[2] = {peer_scb->fcb, new_peer_scb->fcb};
	FCB * fcb2[2] = {new_peer_scb->fcb, peer_scb->fcb};

	PIPE_CB * pipe_cb_1 = init_PIPE_CB(fcb1);
	PIPE_CB * pipe_cb_2 = init_PIPE_CB(fcb2);
	
	
	peer_scb->type = SOCKET_PEER;
	new_peer_scb->type = SOCKET_PEER;
	peer_scb->socket_struct.peer_s->peer_scb = new_peer_scb;
	new_peer_scb->socket_struct.peer_s->peer_scb = peer_scb;

	peer_scb->socket_struct.peer_s->reader_pipe = pipe_cb_1;
	peer_scb->socket_struct.peer_s->writer_pipe = pipe_cb_2;

	new_peer_scb->socket_struct.peer_s->reader_pipe = pipe_cb_2;
	new_peer_scb->socket_struct.peer_s->writer_pipe = pipe_cb_1;


}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	return -1;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	return -1;
}

