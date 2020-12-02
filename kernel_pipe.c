
#include "tinyos.h"
#include "kernel_pipe.h"

static file_ops reader_fops = {
	/**
	 *	Open returns NULL
	 *  Write return -1 
	 *	Only Read and Close are used here
	 */
	.Open = bad_pipe_open,
	.Read = pipe_read,
	.Write = bad_pipe_write,
	.Close = pipe_read_close
};

static file_ops writer_fops = {
	/**
	 *	Open returns NULL 
	 * 	Read returns -1 
	 *	Only Write and Close are used here
	 */
	.Open = bad_pipe_open,
	.Read = bad_pipe_read,
	.Write = pipe_write,
	.Close = pipe_write_close
};

PIPE_CB * init_PIPE_CB(FCB ** fcbs){

	PIPE_CB * pipe = (PIPE_CB *)xmalloc(sizeof(PIPE_CB));

	pipe->reader = fcbs[READ];
	pipe->writer = fcbs[WRITE];

	pipe->has_data = COND_INIT;
	pipe->has_space = COND_INIT;

	pipe->w_pos = 0;
	pipe->r_pos = 0;

	return pipe;
}


int sys_Pipe(pipe_t* pipe)
{
	Fid_t fids[2];
	FCB * fcbs[2];

	if(!FCB_reserve(2, fids, fcbs))
		return -1;
	
	PIPE_CB * pipe_cb = init_PIPE_CB(fcbs);


	pipe->read = fids[READ];
	pipe->write = fids[WRITE];

	fcbs[READ]->streamobj = pipe_cb;
	fcbs[WRITE]->streamobj = pipe_cb;

	fcbs[READ]->streamfunc = &reader_fops;
	fcbs[WRITE]->streamfunc = &writer_fops;

	return 0;
}

void * bad_pipe_open(uint fid){
	return NULL;
}

int bad_pipe_read(void * pipe_cb, char * buffer, unsigned int n){
	return -1;
}

int bad_pipe_write(void * pipe_cb, const char * buffer, unsigned int n){
	return -1;
}

int pipe_read(void * pipe_cb, char * buffer, unsigned int n){
	/* change this */
	return -1;
}

int pipe_write(void * pipe_cb, const char * buffer, unsigned int n){
	/* change this */
	return -1;
}

int pipe_read_close(void * pipe_cb){
	return -1;
}

int pipe_write_close(void * pipe_cb){
	return -1;
}
