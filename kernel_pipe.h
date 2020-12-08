#include "kernel_streams.h"


#define PIPE_BUFFER_SIZE (8 * 1024)
#define READ 0
#define WRITE 1

typedef struct pipe_control_block{
    /**
     * Since a PIPE has 2 file descriptors we need 2 FCBS for each one
     */
    FCB * reader;
    FCB * writer;

    /** Also 2 condition variables for wether the buffer:
     * 1. Need data (ready to read)
     * 2. Need space (ready to write)
     */
    CondVar need_data;
    CondVar need_space;

    /* Read / Write indices on the BUFFER */
    uint w_pos;
    uint r_pos;

    /* The buffer used for Writing / Reading */
    char BUFFER[PIPE_BUFFER_SIZE];
}PIPE_CB;


PIPE_CB * init_PIPE_CB(FCB **);
/* Function for reading from PIPE */
int pipe_read(void *, char *, unsigned int);
/* Close reading end */
int pipe_read_close(void *);
/* Function for writing to the PIPE */
int pipe_write(void *, const char *, unsigned int);
/* Close writing end */
int pipe_write_close(void *);
/* Bad read to be used in file_ops for writer */
int dummy_pipe_read(void *, char *, unsigned int);
/* Bad write to be used in file_ops for reader */
int dummy_pipe_write(void *, const char *, unsigned int);
/* Bad open to be used in both file_ops */
void * dummy_pipe_open(uint);
