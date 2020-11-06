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

// Tid_t sys_CreateThread(Task, int, void*);

// Tid_t sys_ThreadSelf(void);

// int sys_ThreadJoin(Tid_t, int*);

// int sys_ThreadDetach(Tid_t);

// void sys_ThreadExit(int);