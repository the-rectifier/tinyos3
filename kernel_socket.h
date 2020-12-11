#include "kernel_streams.h"
#include "kernel_pipe.h"

typedef struct {
    rlnode request_queue;
    CondVar req_available_cv;
}Listener_socket;

typedef struct {
    rlnode unbound_socket;
}Unbound_socket;

typedef struct {
    SCB * peer_scb;
    PIPE_CB * writer_pipe;
    PIPE_CB * reader_pipe;
}Peer_socket;

typedef struct connection_request{
    int admitted;
    SCB * peer_SCB;
    CondVar connected_cv;
    rlnode queue_node;
}Con_request;


typedef enum {
    SOCKET_LISTENER,
    SOCKET_UNBOUND,
    SOCKET_PEER
}socket_type;

typedef struct socket_control_block{
    uint refcount;
    FCB * fcb;
    socket_type type;
    port_t port;
    union {
        Listener_socket * listener_s;
        Unbound_socket * unbound_s;
        Peer_socket * peer_s;
    }socket_struct;
    
}SCB;
