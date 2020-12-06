
#include "tinyos.h"
#include "kernel_socket.h"

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
	Fid_t  fid;
	FCB * fcb;
	if(!FCB_reserve(1, &fid, &fcb) || (port < 0 || port >= MAX_PORT))
		return NOFILE;

	SCB * socket_cb = init_scb(fcb, port);

	fcb->streamobj = socket_cb;
	fcb->streamfunc = &socket_file_ops;

	return fid;
}

int sys_Listen(Fid_t sock)
{
	return -1;
}


Fid_t sys_Accept(Fid_t lsock)
{
	return NOFILE;
}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	return -1;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	return -1;
}

