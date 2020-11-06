#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_cc.h"
/** 
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{
	//Use some kind of mutex?
	//Check if the current process running is the owner of the current thread
	
	assert(CURPROC == CURTHREAD->owner_pcb);

	CURPROC->thread_count++;
	PTCB * ptcb = init_PTCB(task, argl, args);

	rlnode * ptcb_node = rlnode_init(&ptcb->ptcb_list_node, ptcb);
	rlist_push_back(&CURPROC->ptcb_list, ptcb_node);

	// Just like sys_exec()
	if(task != NULL){
		TCB * tcb = spawn_thread(CURPROC, start_thread);
		ptcb->tcb = tcb;
		tcb->ptcb = ptcb;
		wakeup(ptcb->tcb);
	}
	

	return (Tid_t ) ptcb;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) CURTHREAD;
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
	return -1;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
	return -1;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{

}

PTCB * init_PTCB(Task task, int argl, void* args){
	PTCB * ptcb = (PTCB *)xmalloc(sizeof(PTCB));

	ptcb->task = task;
	ptcb->argl = argl;
	if(args != NULL){
		ptcb->args = malloc(argl);
		memcpy(ptcb->args, args, argl);
	}else{ptcb->args = NULL;}

	ptcb->exitval = -1;
	ptcb->exited = 0;
	ptcb->detached = 0;
	ptcb->exit_cv = COND_INIT;

	ptcb->ref_count = 0;
	
	return ptcb;
}
