#include "types.h"
#include "kernel_streams.h"
#include "kernel_pipe.h"

typedef struct {
    rlnode queue;
    CondVar req_available_cv;
}listener_socket;

typedef struct {
    rlnode unbound_socket;
}unbound_socket;

typedef struct {
    SCB * peer;
    PIPE_CB * writer_pipe;
    PIPE_CB * reader_pipe;
}peer_socket;

typedef struct connection_request{
    int admitted;
    SCB * peer_SCb;
    CondVar connected_cv;
    rlnode queue_node;
}con_request;


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
        listener_socket * listener_s;
        unbound_socket * unbound_s;
        peer_socket * peer_s;
    }socket_struct;
    
}SCB;
