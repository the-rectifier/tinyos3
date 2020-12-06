
#include "tinyos.h"
#include "kernel_pipe.h"
#include "kernel_cc.h"

static file_ops reader_fops = {
	/**
	 *	Open returns NULL
	 *  Write return -1 
	 *	Only Read and Close are used here
	 */
	.Open = dummy_pipe_open,
	.Read = pipe_read,
	.Write = dummy_pipe_write,
	.Close = pipe_read_close
};

static file_ops writer_fops = {
	/**
	 *	Open returns NULL 
	 * 	Read returns -1 
	 *	Only Write and Close are used here
	 */
	.Open = dummy_pipe_open,
	.Read = dummy_pipe_read,
	.Write = pipe_write,
	.Close = pipe_write_close
};

PIPE_CB * init_PIPE_CB(FCB ** fcbs){

	/** 
	 * Create a new PIPE_CB
	 * Assign to the reader/writer fields the correct FCB pointers
	 * Init the Cond Vars
	 * Init the buffer positions
	 * Init the buffer w/ NULL bytes
	 */

	PIPE_CB * pipe = (PIPE_CB *)xmalloc(sizeof(PIPE_CB));

	pipe->reader = fcbs[READ];
	pipe->writer = fcbs[WRITE];

	pipe->need_data = COND_INIT;
	pipe->need_space = COND_INIT;

	pipe->w_pos = 0;
	pipe->r_pos = 0;

	memset(pipe->BUFFER, 0, PIPE_BUFFER_SIZE);

	return pipe;
}


int sys_Pipe(pipe_t* pipe)
{
	/* Create and reserve 2 FIDs and their 2 corresponding FCBs */
	Fid_t fids[2];
	FCB * fcbs[2];

	if(!FCB_reserve(2, fids, fcbs))
		return -1;
	
	/* Initialize the PIPE_CB */
	PIPE_CB * pipe_cb = init_PIPE_CB(fcbs);

	/* Connect the given pipe with the the reserved FIDs */
	pipe->read = fids[READ];
	pipe->write = fids[WRITE];

	/* The stream object is our PIPE_CB */
	fcbs[READ]->streamobj = pipe_cb;
	fcbs[WRITE]->streamobj = pipe_cb;

	/* Assign the corresponding functions to each FCB */
	fcbs[READ]->streamfunc = &reader_fops;
	fcbs[WRITE]->streamfunc = &writer_fops;

	return 0;
}

int pipe_read(void * pipe_cb, char * buffer, unsigned int n){
	size_t bytes;
	PIPE_CB * pipe = (PIPE_CB *)pipe_cb;

	/* If the reader is NULL we obv can't read... */
	if(pipe->reader == NULL){
		return -1;
	/** 
	 * If the write end is CLOSED and the read/write pos match
	 * then nothing to read return 0
	 */
	}else if(pipe->r_pos == pipe->w_pos && pipe->writer == NULL){
		return 0;
	}

	/* Try to read up to n bytes from the pipe */
	for(bytes = 0;bytes<n;bytes++){
		/** If the read/write pos match and the write end is OPEN,
		 * sleep until someone writes to the buffer
		 */
		while(pipe->r_pos == pipe->w_pos && pipe->writer != NULL){
			kernel_broadcast(&pipe->need_space);
			kernel_wait(&pipe->need_data, SCHED_PIPE);
		}
		/** Check if the write end is closed and the read/write pos match 
		 * if they do, it means we reached the end of the buffer before reading
		 * n bytes. Return as many as we read.
		 */
		if(pipe->writer == NULL && pipe->r_pos == pipe->w_pos){
			return (int)bytes;
		}
		/* Copy one byte into the buffer */
		buffer[bytes] = pipe->BUFFER[pipe->r_pos];
		/* Increment the read position and return to start if necessary */
		pipe->r_pos = (pipe->r_pos+1) % PIPE_BUFFER_SIZE;
	}
	kernel_broadcast(&pipe->need_space);
	return (int)bytes;
}

int pipe_write(void * pipe_cb, const char * buffer, unsigned int n){
	size_t bytes;
	PIPE_CB * pipe = (PIPE_CB *)pipe_cb;

	/* If either end is closed return -1 */
	if(pipe->reader == NULL || pipe->writer == NULL){
		return -1;
	}

	/* Try to write n bytes to pipe */
	for(bytes = 0;bytes<n;bytes++){
		/**
		 * If the next write position is the same as the current read it means we have written to
		 * the full pipe. Sleep until somebody reads (reader needs to be open obv).
		 */
		while(pipe->r_pos == (pipe->w_pos + 1) % PIPE_BUFFER_SIZE && pipe->reader != NULL){
			kernel_broadcast(&pipe->need_data);
			kernel_wait(&pipe->need_space, SCHED_PIPE);
		}
		/**
		 * Check if at any time the read/write end is closed
		 * Return -1 in any case
		 */
		if(pipe->reader == NULL || pipe->writer == NULL){
			return (int)bytes;
		}
		/* Write one byte into the pipe */
		pipe->BUFFER[pipe->w_pos] = buffer[bytes];
		/* Increment the write position and return to the start if necessary */
		pipe->w_pos = (pipe->w_pos+1) % PIPE_BUFFER_SIZE;
	}
	kernel_broadcast(&pipe->need_data);
	return (int)bytes;
}

int pipe_read_close(void * pipe_cb){
	PIPE_CB * pipe = (PIPE_CB *) pipe_cb;
	/* Set the reader FCB as NULL */
	pipe->reader = NULL;
	/* Broadcast to anyone that is sleeping on needing space (blocked write) that the read end is closed */
	kernel_broadcast(&pipe->need_space);
	/* If the write part is closed as well destroy the pipe */
	if(pipe->writer == NULL){
		free(pipe);
	}
	return 0;
}

int pipe_write_close(void * pipe_cb){
	PIPE_CB * pipe = (PIPE_CB *) pipe_cb;
	/* Set the writer FCB as NULL */
	pipe->writer = NULL;
	/* Broadcast to anyone that is sleeping on needing data (blocked read) that the write end is closed */
	kernel_broadcast(&pipe->need_data);
	/* if the read end is closed as well destroy the pipe */
	if(pipe->reader == NULL){
		free(pipe);
	}
	return 0;
}

void * dummy_pipe_open(uint fid){
	return NULL;
}

int dummy_pipe_read(void * pipe_cb, char * buffer, unsigned int n){
	return -1;
}

int dummy_pipe_write(void * pipe_cb, const char * buffer, unsigned int n){
	return -1;
}
