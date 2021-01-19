
#include <conf.h>
#include <kernel.h>
#include <stdio.h>
#include <lock.h>
#include <q.h>
#include <proc.h>

extern struct lock locktab[];
extern struct pentry proctab[];
extern int currpid;
extern int numberofreaders;
extern int ctr1000;

int getmaxwriter(int ldes1);

void handleinheritancelock(int ldes1, int pid);


int lock(int ldes1, int type, int priority){
	STATWORD ps;
	disable(ps);
	
	//lock is not in the list of locks or the lock has been deleted
	if(locktab[ldes1].state == UNUSED || proctab[currpid].ldeleted == 1){
		restore(ps);
		return SYSERR;
	}

	//lock is present but it had been deleted and replaced by another lock
	//-----------


	//Set the process type in proctab
	//proctab[currpid].locks[ldes1] = type;

	//the process is of any type and lock is available
	if(locktab[ldes1].state == AVAILABLE){
		locktab[ldes1].state = ACQUIRED;
		locktab[ldes1].type = type;
		proctab[currpid].locks[ldes1] = type;
		locktab[ldes1].pacq[currpid] = type;
		proctab[currpid].waittime = 0;
		proctab[currpid].waittype = -1;
		proctab[currpid].waitqueue = -1;
		if(type ==  READ){
			//numberofreaders++;
			locktab[ldes1].readers++;
		}
		restore(ps);
		return(OK);
	}

	//the process is of type read and lock is acquired
	if(locktab[ldes1].state == ACQUIRED && locktab[ldes1].type == READ){
		if(type == READ){ //requesting lock is read
			//give the lock to the reader if there is no higher priority writer waiting
			int maxwriter = getmaxwriter(ldes1);
			if(maxwriter > priority){//higher prio wirter exists
				//make the process wait
				proctab[currpid].pstate = PRWAIT;
				//update priorities
				handleinheritancelock(ldes1, currpid);
				//insert the process in the wait list
				insert(currpid, locktab[ldes1].head, priority);
				//set the waittime to ctr1000 and waittype to the type of lock request
				proctab[currpid].waittime = ctr1000;
				proctab[currpid].waittype = type;
				proctab[currpid].waitqueue = ldes1;

				//remove the process from the wait queue, is this needed?
				//-----------
				resched();
				if(locktab[ldes1].state == UNUSED){
					restore(ps);
					return(DELETED);
				}
				restore(ps);
				return(OK);
				
				
				return(OK);
			}
			else if(maxwriter == priority){ //max prio writer equal to current prio
				if(ctr1000 - proctab[maxwriter].waittime <= 40){ //reader requesting within 400 millisec
					//assign lock
					locktab[ldes1].type = type;
					proctab[currpid].locks[ldes1] = type;
					locktab[ldes1].pacq[currpid] = type;
					proctab[currpid].waittime = 0;
					proctab[currpid].waittype = -1;
					proctab[currpid].waitqueue = -1;
					//numberofreaders++;
					locktab[ldes1].readers++;

					restore(ps);
					return(OK);
				}
			}
			else{
				//give lock
				locktab[ldes1].type = type;
				proctab[currpid].locks[ldes1] = type;
				locktab[ldes1].pacq[currpid] = type;
				proctab[currpid].waitqueue = -1;
				//numberofreaders++;
				locktab[ldes1].readers++;

				restore(ps);
				return(OK);
			}
		}
		else{
			//requesting lock is write
			//make the process wait
			proctab[currpid].pstate = PRWAIT;
			//update priorities
			handleinheritancelock(ldes1, currpid);
			//insert the process in the wait list
			insert(currpid, locktab[ldes1].head, priority);
			//set the waittime to ctr1000 and waittype to the type of lock request
			proctab[currpid].waittime = ctr1000;
			proctab[currpid].waittype = type;
			proctab[currpid].waitqueue = ldes1;

			resched();
			if(locktab[ldes1].state == UNUSED){
				restore(ps);
				return(DELETED);
			}
			restore(ps);
			return(OK);
			
			return(OK);
		}

			
	}

	//the process is of type write and the lock is acquired
	if(locktab[ldes1].state == ACQUIRED && locktab[ldes1].type == WRITE){
		//doesn't matter what the incoming process is, it has to wait
		//make the process wait
		proctab[currpid].pstate = PRWAIT;
		//update priorities
		handleinheritancelock(ldes1, currpid);
		//insert the process in the wait list
		insert(currpid, locktab[ldes1].head, priority);
		//set the waittime to ctr1000;
		proctab[currpid].waittime = ctr1000;
		proctab[currpid].waittype = type;
		proctab[currpid].waitqueue = ldes1;

		resched();
		if(locktab[ldes1].state == UNUSED){
			restore(ps);
			return(DELETED);
		}
		restore(ps);
		return(OK);
	}
}

void handleinheritancelock(int ldes1, int pid){
	//check if the priority of the waiting process is higher than the max prio of the processes holding the lock
	int i;
	for(i=0; i<NPROC; i++){
		//check if the processes that hold this lock have a lower prio than the waiting process
		if(locktab[ldes1].pacq[i] != UNUSED && proctab[i].pprio < proctab[pid].pprio){
			proctab[i].pprio = proctab[pid].pprio;
			if(proctab[i].waitqueue != -1){
				handleinheritancelock(proctab[i].waitqueue, i);	
			}
			
		}
	}
}

//returns the max prio writer in the queue
int getmaxwriter(int ldes1){
	int maxwriter = MININT;
	int next = q[locktab[ldes1].head].qnext;
	while(next != locktab[ldes1].tail){
		if(proctab[next].waittype == WRITE && q[next].qkey >= maxwriter){
			maxwriter = q[next].qkey;
		}
		next = q[next].qnext;
	}
	return maxwriter;
}