
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
// #include "kernel_threads.h"

/** 
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{
	return NOTHREAD;
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
	PTCB * ptcb = (PTCB *)malloc(sizeof(PTCB));

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
}
