#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_cc.h"

/** 
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{
	if(task == NULL){
		return NOTHREAD;
	}

	// Just like sys_exec()
	// new thread on the process
	CURPROC->thread_count++;
	// grab new ptcb
	PTCB * ptcb = init_PTCB(task, argl, args);

	// push back the ptcb 
	rlist_push_back(&CURPROC->ptcb_list, &ptcb->ptcb_list_node);
	// spawn the new thread and make neccessary connetions
	TCB * tcb = spawn_thread(CURPROC, start_thread);
	ptcb->tcb = tcb;
	tcb->ptcb = ptcb;

	// wake up thread
	wakeup(ptcb->tcb);

	return (Tid_t ) ptcb;
}

/**
  @brief Return the Tid of the current PTCB
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) cur_thread()->ptcb;
}

/**
  @brief Join the given thread.
  An exited thread needs another to join it in order to be freed
  Otherwise the PTCBs are freed by the last thread of the process when exiting
  Return 0 on SUCCESS -1 on FAILURE
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
	// try to find the PTCB 
	rlnode* node = rlist_find(&(cur_thread()->owner_pcb->ptcb_list), (PTCB *)tid, NULL);
	if(node == NULL){
		return -1;
	}
	PTCB* ptcb = node->ptcb;

	// PTCB* ptcb = (PTCB *)tid;

	//assert(ptcb != NULL);

	// can't join a detached thread
	// can't join our selves ffs....
	// can't join a thread with no ptcb obv...
	if((ptcb == NULL) || (ptcb->detached) || (ptcb->tcb == cur_thread())){return -1;}
	else{
		// one more to the waiting list
		ptcb->ref_count++;
		// sleep until thread exits or thread is detached
		while(!ptcb->exited && !ptcb->detached){
			kernel_wait(&ptcb->exit_cv, SCHED_USER);
		}
		// check if thread exited w/ detached status
		ptcb->ref_count--;
		if(ptcb->detached){
			return -1;
		}else{
			// set any exit value
			if(exitval){
				*(exitval) = ptcb->exitval;
			}
			// if last waiting then clear ptcb 
			// might change
			if(ptcb->ref_count < 1){
				rlist_remove(&ptcb->ptcb_list_node);
				free(ptcb);
			}
			return 0;
		}

	}
	
}

/**
  @brief Detach the given thread.
  A detached thread is freed on exit from the last process thread.
  A thread can detach itself
  */
int sys_ThreadDetach(Tid_t tid)
{

	rlnode* node = rlist_find(&(cur_thread()->owner_pcb->ptcb_list), (PTCB *)tid, NULL);
	if(node == NULL){
		return -1;
	}
	PTCB* ptcb = node->ptcb;


	// assert(ptcb != NULL);
	// cant detach an exited thread
	if(ptcb == NULL || ptcb->exited){
		return -1;
	}else{
		// Mark PTCB as detached
		ptcb->detached = 1;
		// A detached thread cant be waited from anyone
		// Mark it's references as 0
		ptcb->ref_count = 0;
		// Broadcast for detachment
		kernel_broadcast(&ptcb->exit_cv);
		return 0;
	}

	return -1;
}

/**
 * @brief Terminate the current thread.
 * The last thread cleans up the PTCB list of the process
 */
void sys_ThreadExit(int exitval)
{	
	kill_thread(exitval);

	if(CURPROC->thread_count == 0){
		PCB * curproc = CURPROC;
		
		// cannot reparent init 
		// from the new updates after Oct 2020
		if(get_pid(curproc) != 1){
			/* Reparent any children of the exiting process to the 
			initial task */
			PCB* initpcb = get_pcb(1);
			while(!is_rlist_empty(& curproc->children_list)) {
				rlnode* child = rlist_pop_front(& curproc->children_list);
				child->pcb->parent = initpcb;
				rlist_push_front(& initpcb->children_list, child);
			}

			/* Add exited children to the initial task's exited list 
			and signal the initial task */
			if(!is_rlist_empty(& curproc->exited_list)) {
				rlist_append(& initpcb->exited_list, &curproc->exited_list);
				kernel_broadcast(& initpcb->child_exit);
			}
			/* Put me into my parent's exited list */
			rlist_push_front(& curproc->parent->exited_list, &curproc->exited_node);
			kernel_broadcast(& curproc->parent->child_exit);
		}

		assert(is_rlist_empty(& curproc->children_list));
		assert(is_rlist_empty(& curproc->exited_list));
	
		/* 
			Do all the other cleanup we want here, close files etc. 
		*/

		/* Release the args data */
		if(curproc->args) {
			free(curproc->args);
			curproc->args = NULL;
		}

		/* Clean up FIDT */
		for(int i=0;i<MAX_FILEID;i++){
			if(curproc->FIDT[i] != NULL){
				FCB_decref(curproc->FIDT[i]);
				curproc->FIDT[i] = NULL;
			}
		}

		/* cleanup detached threads */
		while(!is_rlist_empty(&curproc->ptcb_list)){
			PTCB * ptcb_temp = rlist_pop_front(&curproc->ptcb_list)->ptcb;
			assert(ptcb_temp != NULL);
			free(ptcb_temp);
		}

		/* Disconnect my main_thread */
		curproc->main_thread = NULL;

		/* Now, mark the process as exited. */
		curproc->pstate = ZOMBIE;
	}
	
	/* Bye-bye cruel world */
	kernel_sleep(EXITED, SCHED_USER);
}

void kill_thread(int exitval){
	PTCB * ptcb = cur_thread()->ptcb;
	// Mark thread as exited and pass the exit value
	ptcb->exited = 1;
	ptcb->exitval = exitval;
	// Broadcast
	kernel_broadcast(&ptcb->exit_cv);
	CURPROC->thread_count--;
}

PTCB * init_PTCB(Task task, int argl, void* args){
	PTCB * ptcb = (PTCB *)xmalloc(sizeof(PTCB));

	ptcb->task = task;
	ptcb->argl = argl;
	ptcb->args = args;

	ptcb->exitval = 0;
	ptcb->exited = 0;
	ptcb->detached = 0;
	ptcb->exit_cv = COND_INIT;

	ptcb->ref_count = 0;

	rlnode_init(&ptcb->ptcb_list_node, ptcb);
	
	return ptcb;
}
