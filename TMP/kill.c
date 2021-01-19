/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */

void handleinheritancekill(int ldes1, int pid);
int getmaxp(int ldes1, int pid);


SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	int l;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}


	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	
					//release the locks held by the killed process
					
					for(l=0; l<0; l++){
						if(proctab[pid].locks[l] != UNUSED){
							//release all the locks
						}
					}

					//if the killed process is waiting in a queue, update the prios of procs holding the lock 
					int p;
					int maxprio = 0;
					int wqueue = proctab[pid].waitqueue;

					if(wqueue != -1){
						//get the max prio in the waiting queue
						int next = q[locktab[wqueue].head].qnext;
						while(next != locktab[wqueue].tail){
							if((next != pid) && (proctab[next].pprio > maxprio)){	//check all the processes except the killed process
								maxprio = proctab[next].pprio;
							}
							next = q[next].qnext;
						}
						
						for(p=0; p<NPROC; p++){
							if(locktab[wqueue].pacq[p] != UNUSED){
								//set the value to the actual priority
								proctab[p].pprio = proctab[p].actprio;
							}
						}
						
						for(p=0; p<NPROC; p++){
							if((locktab[wqueue].pacq[p] != UNUSED) && (p != pid)){
								//now take care of inheritance
								//first get the prio boosted by waiting processes
								int j;
								int maxp;
								for(j = 0; j<NLOCKS; j++){
									if(proctab[p].locks[j] != UNUSED){
										maxp = getmaxp(j, pid);
										if(maxp > proctab[p].pprio){
											proctab[p].pprio = maxp;
										}
									}
								}

								//now that the priority is boosted, if p is waiting in a queue, handle inheritance
								if(proctab[p].waitqueue != -1){
									handleinheritancekill(proctab[p].waitqueue, p);
								}
							}
						}
					}
					semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	restore(ps);
	return(OK);
}


void handleinheritancekill(int ldes1, int pid){
	//check if the priority of the waiting process is higher than the max prio of the processes holding the lock
	int i;
	for(i=0; i<NPROC; i++){
		//check if the processes that hold this lock have a lower prio than the waiting process
		if(locktab[ldes1].pacq[i] != UNUSED && proctab[i].pprio < proctab[pid].pprio){
			proctab[i].pprio = proctab[pid].pprio;
			if(proctab[i].waitqueue != -1){
				handleinheritancekill(proctab[i].waitqueue, i);	
			}
			
		}
	}
}

int getmaxp(int ldes1, int pid){
	int maxprio = 0;
	int next = q[locktab[ldes1].head].qnext;
	while(next != locktab[ldes1].tail){
		if((next != pid) && (proctab[next].pprio > maxprio)){
			maxprio = proctab[next].pprio;
		}
		next = q[next].qnext;
	}
	return maxprio;
}
