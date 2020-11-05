#include <kernel_sched.h>

typedef struct process_thread_control_block{
	TCB* tcb;

	Task task;
	int argl;
	void *args;

	int exitval;

	int exited;
	int detached;
	CondVar exit_cv;

	int ref_count;

	rlnode ptcb_list_node;
}PTCB;

PTCB * init_PTCB(Task, int, void*);
