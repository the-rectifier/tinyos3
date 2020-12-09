#include "kernel_streams.h"
#include "kernel_pipe.h"

typedef struct socket_control_block SCB;

enum socket_type{
    SOCKET_PEER,
    SOCKET_UNBOUND,
    SOCKET_LISTENER
};

typedef struct listener_socket{
    CondVar req_available;
    rlnode req_queue;
}lsock_t;

typedef struct peer_socket{
    SCB* peer;
    PIPE_CB* write_pipe;
    PIPE_CB* read_pipe;
}psock_t;

typedef struct connection_request{
    int admitted;
    SCB * peer;
    CondVar connected_cv;
    rlnode queue_node;
}request_t;

struct socket_control_block{
    uint refcount;
    FCB* fcb;
    port_t port;
    enum socket_type type;

    union{
        lsock_t * listener_s;
        psock_t * peer_s;
    }props;
};


SCB * init_SCB(FCB *, port_t);
SCB * get_scb(Fid_t);
request_t * craft_request(SCB *);
void * dummy_socket_open(uint);
int socket_read(void*, char *, unsigned int);
int socket_write(void*, const char *, unsigned int);
int socket_close(void *);
void SCB_decref(SCB *);
void socket_close_read(SCB * scb);
void socket_close_write(SCB * scb);
