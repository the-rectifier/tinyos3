
#include "util.h"

void print_hex(unsigned char *data, size_t len){
	size_t i;
	if(!data){
		printf("NULL data\n");
	}else{
		for(i = 0; i < len; i++){
			if(!(i % 8) && (i!= 0)){
		    	printf(" ");
			}
			if(!(i % 16) && (i != 0)){
				printf("\n");
			}
			printf("%02X ", data[i]);
		}
		printf("\n");
	}
}


void raise_exception(exception_context context)
{
	if(*context) {
		__atomic_signal_fence(__ATOMIC_SEQ_CST);
		longjmp((*context)->jbuf, 1);
	}
}

void exception_unwind(exception_context context, int errcode)
{
	/* Get the top frame */
	struct exception_stack_frame* frame = *context;

	/* handle exception */
	int captured = 0;

	/* First execute catchers one by one */
	while(frame->catchers) {
		captured = 1;
		struct exception_handler_frame *c = frame->catchers;
		/* Pop it from the list, just in case it throws() */
		frame->catchers = c->next;
		c->handler(errcode);
	}

	/* Execute finalizers one by one */
	while(frame->finalizers) {
		struct exception_handler_frame *fin = frame->finalizers;
		frame->finalizers = fin->next;

		fin->handler(errcode);
	}
 	
	/* pop this frame */
	*context = frame->next;

 	/* propagate */
	if(errcode && !captured) 
		raise_exception(context);
}

