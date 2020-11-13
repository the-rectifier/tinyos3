#include <kernel_sched.h>
#include <kernel_streams.h>

/*
	Intermediate data structure to achieve
	multithreading implenentation 
*/
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

/*
	Initializes the PTCB structure and also its intrusive node
	returns a ptr to the newly created PTCB
*/
PTCB * init_PTCB(Task, int, void*);

/*
	Marks a PTCB as exited 
	Decrements it's PCB's thread counter
	Broadcasts to all listeners for a death in the family 
	*SPOILERS* robin dies :(
*/
void kill_thread(int);

// Tid_t sys_CreateThread(Task, int, void*);

// Tid_t sys_ThreadSelf(void);

// int sys_ThreadJoin(Tid_t, int*);

// int sys_ThreadDetach(Tid_t);

// void sys_ThreadExit(int);