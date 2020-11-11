#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_cc.h"
/** 
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{
	
	PTCB * ptcb = NOTHREAD;

	//Check if the current process running is the owner of the current thread
	assert(CURPROC == cur_thread()->owner_pcb);

	// Just like sys_exec()
	if(task != NULL){
		// new thread on the process
		CURPROC->thread_count++;
		// grab new ptcb
		ptcb = init_PTCB(task, argl, args);

		// push back the ptcb 
		rlist_push_back(&CURPROC->ptcb_list, &ptcb->ptcb_list_node);
		// spawn the new thread and make neccessary connetions
		TCB * tcb = spawn_thread(CURPROC, start_thread);
		ptcb->tcb = tcb;
		tcb->ptcb = ptcb;

		// wake up thread
		wakeup(ptcb->tcb);
	}

	return (Tid_t ) ptcb;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) cur_thread();
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
	// try to find the PCB 
	PTCB * ptcb = rlist_find(&(cur_thread()->owner_pcb->ptcb_list), tid, NULL);
	// assert(ptcb != NULL);

	// can't join a detached thread
	// can't join our selves ffs....
	// can't join a thread with no ptcb obv...
	if((ptcb == NULL) || (ptcb->detached) || (ptcb->tcb == cur_thread())){return -1;}
	else{
		// one more to the waiting list
		ptcb->ref_count++;
		// sleep until thread exits or thread is detached
		while(!ptcb->exited && !ptcb->detached){
			kernel_wait(&ptcb->exit_cv, SCHED_MUTEX);
		}
		// check if thread exited w/ detached status
		if(ptcb->detached){
			return -1;
		}else{
			// set any exit value
			if(exitval){
				*(exitval) = ptcb->exitval;
			}
			// if last waiting then clear ptcb might change
			if(ptcb->ref_count == 0){
				rlist_remove(&ptcb->ptcb_list_node);
				free(ptcb);
			}
		}

	}
	
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
	
	rlnode_init(&ptcb->ptcb_list_node, ptcb);

	ptcb->task = task;
	ptcb->argl = argl;
	if(args != NULL){
		ptcb->args = xmalloc(argl);
		memcpy(ptcb->args, args, argl);
	}else{ptcb->args = NULL;}

	ptcb->exitval = -1;
	ptcb->exited = 0;
	ptcb->detached = 0;
	ptcb->exit_cv = COND_INIT;

	ptcb->ref_count = 0;
	
	return ptcb;
}
