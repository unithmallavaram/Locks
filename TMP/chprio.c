/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */

int getmax(int ldes1);
void handleinheritancechange(int ldes1, int pid);

SYSCALL chprio(int pid, int newprio)
{
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	if (isbadpid(pid) || newprio<=0 ||
	    (pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		return(SYSERR);
	}

	int oldprio = pptr->pprio;

	pptr->pprio = newprio;
	pptr->actprio = newprio;


	//boost the prio due to the waiting processes in locks held by currpid
	int l;
	int maxprio = newprio;
	int tempprio;
	for(l=0; l<NLOCKS; l++){
		if(proctab[currpid].locks[l] != UNUSED){
			tempprio = getmax(l);
			if(tempprio > maxprio){
				maxprio = tempprio;
			}
		}
	}

	pptr->pprio = maxprio;

	//now update priorities 
	if(proctab[currpid].waitqueue != -1){
		handleinheritancechange(proctab[currpid].waitqueue, currpid);	
	}
	
	restore(ps);
	return(newprio);
	
}

void handleinheritancechange(int ldes1, int pid){
	//check if the priority of the waiting process is higher than the max prio of the processes holding the lock
	int i;
	for(i=0; i<NPROC; i++){
		//check if the processes that hold this lock have a lower prio than the waiting process
		if(locktab[ldes1].pacq[i] != UNUSED && proctab[i].pprio < proctab[pid].pprio){
			proctab[i].pprio = proctab[pid].pprio;
			if(proctab[i].waitqueue != -1){
				handleinheritancechange(proctab[i].waitqueue, i);
			}
			
		}
	}
}

int getmax(int ldes1){
	int maxprio = 0;
	int next = q[locktab[ldes1].head].qnext;
	while(next != locktab[ldes1].tail){
		if(proctab[next].pprio > maxprio){
			maxprio = proctab[next].pprio;
		}
		next = q[next].qnext;
	}
	return maxprio;
}
