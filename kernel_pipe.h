#include "kernel_streams.h"


#define PIPE_BUFFER_SIZE 1024
#define READ 0
#define WRITE 1

typedef struct pipe_control_block{
    /**
     * Since a PIPE has 2 file descriptors we need 2 FCBS for each one
     */
    FCB * reader;
    FCB * writer;

    /** Also 2 condition variables for wether the buffer:
     * 1. Has data (ready to read)
     * 2. Has space (ready to write)
     */
    CondVar has_data;
    CondVar has_space;

    /* Read / Write indices on the BUFFER */
    unsigned int w_pos;
    unsigned int r_pos;

    /* The buffer used for Writing / Reading */
    char BUFFER[PIPE_BUFFER_SIZE];
}PIPE_CB;


/* Function for reading from PIPE */
int pipe_read(void *, const char *, unsigned int);
/* Close reading end */
int pipe_read_close(void *);
/* Function for writing to the PIPE */
int pipe_write(void *, const char *, unsigned int);
/* Close writing end */
int pipe_write_close(void *);
/* Bad read to be used in file_ops for writer */
int bad_pipe_read(void *, const char *, unsigned int);
/* Bad write to be used in file_ops for reader */
int bad_pipe_write(void *, const char *, unsigned int);
/* Bad open to be used in both file_ops */
void * bad_pipe_open(void *);
