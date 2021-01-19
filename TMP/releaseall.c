
#include <conf.h>
#include <kernel.h>
#include <stdio.h>
#include <lock.h>
#include <q.h>
#include <proc.h>

extern struct lock locktab[];
extern int currpid;
extern struct pentry proctab[];
extern int ctr1000;
extern int numberofreaders;

void handleinheritancerelease(int ldes1, int pid);
void allocatelock(int ldes1);
int getmaxprio(int ldes1);
void handleallocation(int ldes1, int prev, int head, int maxprio);
void handleallocationread(int ldes1, int prev, int head, int maxprio, int maxproc);
void assignlock(int ldes1, int pid, int type);

int releaseall(int numlocks, int ldes1, ...){
	STATWORD ps;
	disable(ps);

	int *argaddress = &numlocks;
	int lockargs[numlocks];
	int l;
	int lerror = 0;

	for(l = 0; l<numlocks; l++){
		lockargs[l] = *(argaddress + 1);
		argaddress += 1;
	}


	// for each lock, check and release
	for(l = 0; l<numlocks; l++){
		if(proctab[currpid].locks[lockargs[l]] != 0){ //lock is actually held by the process
			//release the lock
			int releasetype = proctab[currpid].locks[lockargs[l]];
			if(proctab[currpid].locks[lockargs[l]] == READ){
				//numberofreaders--;
				locktab[lockargs[l]].readers--;
			}
			proctab[currpid].locks[lockargs[l]] = UNUSED;
			locktab[lockargs[l]].pacq[currpid] = UNUSED;

			//after releasing, update the priorities
			//set the prio of the current process to the max of all the waiting processess in the locks acquired by currpid
			int k;
			//set the prio to actual prio and change if there is a higher prio process waiting
			proctab[currpid].pprio = proctab[currpid].actprio;
			int next;
			for(k = 0; k<NLOCKS; k++){
				if(proctab[currpid].locks[k] != UNUSED){
					next = q[locktab[k].head].qnext;
					while(next != locktab[k].tail){
						if(proctab[next].pprio > proctab[currpid].pprio){
							proctab[currpid].pprio = proctab[next].pprio;
						}
						next = q[next].qnext;
					}
				}
			}

			//update priorities for the processes in the queue that the curr process is waiting
			//this step is not necessary as the current process wouldn't be in a wait queue as it is currently executing
			//-------------------------------------
			if(proctab[currpid].waitqueue != -1){
				handleinheritancerelease(proctab[currpid].waitqueue, currpid);
			}
			//-------------------------------------

			//allocate the lock to the eligible processes if the 
			//lock was released by either a writer or the last reader
			if(locktab[lockargs[l]].readers == 0 || releasetype == WRITE){
				allocatelock(lockargs[l]);	
			}

		}
		else{
			lerror = 1;
		}
	}

	if(!lerror){
		restore(ps);
		return(OK);
	}
	else{
		restore(ps);
		return(SYSERR);
	}

}

void handleinheritancerelease(int ldes1, int pid){
	//check if the priority of the waiting process is higher than the max prio of the processes holding the lock
	int i;
	for(i=0; i<NPROC; i++){
		//check if the processes that hold this lock have a lower prio than the waiting process
		if(locktab[ldes1].pacq[i] != UNUSED && proctab[i].pprio < proctab[pid].pprio){
			proctab[i].pprio = proctab[pid].pprio;
			if(proctab[i].waitqueue != -1){
				handleinheritancerelease(proctab[i].waitqueue, i);	
			}
			
		}
	}
}

void allocatelock(int ldes1){
	//find the highest lock priority process
	//keep checking for equal priorities
	int head = locktab[ldes1].head;
	int tail = locktab[ldes1].tail;
	int prev = q[tail].qprev;
	int maxprio = -1;
	int maxproc = -1;
	int type = -1;

	int releasewriter = 0;

	if(prev != head){	//if the queue is not empty
		maxprio = q[prev].qkey;
		maxproc = prev;
		type = proctab[prev].waittype;
	}

	if(maxproc != -1){  //non-empty queue
		int prev = q[tail].qprev;
		//traverse from tail and get the oldest high prio process.
		while(prev != head){
			if(q[prev].qkey == maxprio){ //oldest high prio process
				maxproc = prev;
				break;
			}
			prev = q[prev].qprev;
		}

		//if it is a writer, keep traversing until you find a reader, 
		// then compare for the wait diff of 400 and assign accordingly.
		if(proctab[maxproc].waittype == WRITE){
			//pointing to the entry to the left of oldest max prio proc
			prev = q[prev].qprev;
			while(prev != head){
				if(proctab[prev].waittype == WRITE){
					//assign lock to the writer
					locktab[ldes1].pacq[maxproc] = WRITE;
					locktab[ldes1].type = WRITE;
					proctab[maxproc].locks[ldes1] = WRITE;
					proctab[maxproc].waittime = 0;
					proctab[maxproc].waittype = 0;
					proctab[maxproc].waitqueue = -1;
					//wake the process up
					dequeue(maxproc);

					//update the priorities before wakeup
					proctab[currpid].pprio = proctab[currpid].actprio;
					int maxprio = getmaxprio(ldes1);
					if(maxprio > proctab[currpid].pprio){
						proctab[currpid].pprio = maxprio;
						if(proctab[currpid].waitqueue != -1){
							handleinheritancerelease(proctab[currpid].waitqueue, currpid);
						}
					}

					ready(maxproc, RESCHNO);
					break;
				}
				if(proctab[prev].waittype == READ){
					if(q[prev].qkey < maxprio){
						//assign lock to the writer
						locktab[ldes1].pacq[maxproc] = WRITE;
						locktab[ldes1].type = WRITE;
						proctab[maxproc].locks[ldes1] = WRITE;
						proctab[maxproc].waittime = 0;
						proctab[maxproc].waittype = 0;
						proctab[maxproc].waitqueue = -1;
						//wake the process up
						dequeue(maxproc);

						//update the priorities before wakeup
						proctab[currpid].pprio = proctab[currpid].actprio;
						int maxprio = getmaxprio(ldes1);
						if(maxprio > proctab[currpid].pprio){
							proctab[currpid].pprio = maxprio;
							if(proctab[currpid].waitqueue != -1){
								handleinheritancerelease(proctab[currpid].waitqueue, currpid);
							}
						}

						ready(maxproc, RESCHNO);
						break;
					}
					else {
						if((proctab[prev].waittime - proctab[maxproc].waittime) > 40){
							//assign lock to the writer
							locktab[ldes1].pacq[maxproc] = WRITE;
							locktab[ldes1].type = WRITE;
							proctab[maxproc].locks[ldes1] = WRITE;
							proctab[maxproc].waittime = 0;
							proctab[maxproc].waittype = 0;
							proctab[maxproc].waitqueue = -1;
							//wake the process up
							dequeue(maxproc);

							//update the priorities before wakeup
							proctab[currpid].pprio = proctab[currpid].actprio;
							int maxprio = getmaxprio(ldes1);
							if(maxprio > proctab[currpid].pprio){
								proctab[currpid].pprio = maxprio;
								if(proctab[currpid].waitqueue != -1){
									handleinheritancerelease(proctab[currpid].waitqueue, currpid);
								}
							}

							ready(maxproc, RESCHNO);
							break;
						}
						else{
							//assign lock to the reader
							locktab[ldes1].pacq[prev] = READ;
							locktab[ldes1].type = READ;
							proctab[prev].locks[ldes1] = READ;
							proctab[prev].waittime = 0;
							proctab[prev].waittype = 0;
							proctab[prev].waitqueue = -1;
							//numberofreaders++;
							locktab[ldes1].readers++;
							//wake the process up
							prev = q[maxproc].qprev;
							dequeue(prev);

							//update the priorities before wakeup
							proctab[currpid].pprio = proctab[currpid].actprio;
							int maxprio = getmaxprio(ldes1);
							if(maxprio > proctab[currpid].pprio){
								proctab[currpid].pprio = maxprio;
								if(proctab[currpid].waitqueue != -1){
									handleinheritancerelease(proctab[currpid].waitqueue, currpid);
								}
							}

							ready(prev, RESCHNO);
							handleallocationread(ldes1, prev, head, maxprio, maxproc);
							break;
						}
					}
				}
				prev = q[prev].qprev;
			}
			//there is only one maxproc and it is a writer
			//assign lock to the writer
			locktab[ldes1].pacq[maxproc] = WRITE;
			locktab[ldes1].type = WRITE;
			proctab[maxproc].locks[ldes1] = WRITE;
			proctab[maxproc].waittime = 0;
			proctab[maxproc].waittype = 0;
			proctab[maxproc].waitqueue = -1;
			//wake the process up
			dequeue(maxproc);

			//update the priorities before wakeup
			proctab[currpid].pprio = proctab[currpid].actprio;
			int maxprio = getmaxprio(ldes1);
			if(maxprio > proctab[currpid].pprio){
				proctab[currpid].pprio = maxprio;
				if(proctab[currpid].waitqueue != -1){
					handleinheritancerelease(proctab[currpid].waitqueue, currpid);
				}
			}

			ready(maxproc, RESCHNO);

		}
		else{ 	//if it is a reader, keep traversing until the head and assign all the readers
			while(prev != head){
				if(proctab[prev].waittype == READ){
					//assign lock to the reader
					locktab[ldes1].pacq[prev] = READ;
					locktab[ldes1].type = READ;
					proctab[prev].locks[ldes1] = READ;
					proctab[prev].waittime = 0;
					proctab[prev].waittype = 0;
					proctab[prev].waitqueue = -1;
					//numberofreaders++;
					locktab[ldes1].readers++;
					//wake the process up
					int wake = prev;
					prev = tail;
					dequeue(wake);

					//update the priorities before wakeup
					proctab[currpid].pprio = proctab[currpid].actprio;
					int maxprio = getmaxprio(ldes1);
					if(maxprio > proctab[currpid].pprio){
						proctab[currpid].pprio = maxprio;
						if(proctab[currpid].waitqueue != -1){
							handleinheritancerelease(proctab[currpid].waitqueue, currpid);
						}
					}

					ready(wake, RESCHNO);
				}
				else if(proctab[prev].waittype == WRITE){	//if a writer is encountered, check for 400 with the previous readers and assign accordingly
					handleallocation(ldes1, prev, head, maxprio);
					break;
				}
				prev = q[prev].qprev;
			}
		}
	}
	else{ //empty queue
		locktab[ldes1].state = AVAILABLE;
	}

}

void handleallocationread(int ldes1, int prev, int head, int maxprio, int maxproc){
	prev = q[prev].qprev;
	while(prev != head){
		if((q[prev].qkey == maxprio) && proctab[prev].waittype == READ && (proctab[prev].waittime - proctab[maxproc].waittime) > 40){
			//reader falling in the grace period
			//assign lock to the reader
			locktab[ldes1].pacq[prev] = READ;
			locktab[ldes1].type = READ;
			proctab[prev].locks[ldes1] = READ;
			proctab[prev].waittime = 0;
			proctab[prev].waittype = 0;
			proctab[prev].waitqueue = -1;
			//numberofreaders++;
			locktab[ldes1].readers++;
			//wake the process up
			int wake = prev;
			prev = maxproc;
			dequeue(wake);

			//update the priorities before wakeup
			proctab[currpid].pprio = proctab[currpid].actprio;
			int maxprio = getmaxprio(ldes1);
			if(maxprio > proctab[currpid].pprio){
				proctab[currpid].pprio = maxprio;
				if(proctab[currpid].waitqueue != -1){
					handleinheritancerelease(proctab[currpid].waitqueue, currpid);
				}
			}

			ready(wake, RESCHNO);
		}
		if(proctab[prev].waittype == WRITE)
			break;
		prev = q[prev].qprev;
	}
}

void handleallocation(int ldes1, int prev, int head, int maxprio){
	int maxwriter = prev;
	while(prev != head){
		if((q[prev].qkey == maxprio) && proctab[prev].waittype == READ && (proctab[prev].waittime - proctab[q[maxwriter].qprev].waittime > 40)){
			//assign lock to all the readers falling in the grace period
			locktab[ldes1].pacq[prev] = READ;
			locktab[ldes1].type = READ;
			proctab[prev].locks[ldes1] = READ;
			proctab[prev].waittime = 0;
			proctab[prev].waittype = 0;
			proctab[prev].waitqueue = -1;
			//numberofreaders++;
			locktab[ldes1].readers++;
			//wake the process up
			int wake = prev;
			prev = maxwriter;
			dequeue(wake);

			//update the priorities before wakeup
			proctab[currpid].pprio = proctab[currpid].actprio;
			int maxprio = getmaxprio(ldes1);
			if(maxprio > proctab[currpid].pprio){
				proctab[currpid].pprio = maxprio;
				if(proctab[currpid].waitqueue != -1){
					handleinheritancerelease(proctab[currpid].waitqueue, currpid);
				}
			}

			ready(wake, RESCHNO);
		}
		prev = q[prev].qprev;
	}
}

void assignlock(int ldes1, int pid, int type){
	locktab[ldes1].pacq[pid] = type;
	locktab[ldes1].type = type;
	proctab[pid].locks[ldes1] = type;
	proctab[pid].waittime = 0;
	proctab[pid].waittype = 0;
	proctab[pid].waitqueue = -1;
	if(type == READ)
		//numberofreaders++;
		locktab[ldes1].readers++;
	//wake the process up
	dequeue(pid);

	//update the priorities before wakeup
	proctab[currpid].pprio = proctab[currpid].actprio;
	int maxprio = getmaxprio(ldes1);
	if(maxprio > proctab[currpid].pprio){
		proctab[currpid].pprio = maxprio;
		if(proctab[currpid].waitqueue != -1){
			handleinheritancerelease(proctab[currpid].waitqueue, currpid);
		}
	}

	ready(pid, RESCHNO);
}

int getmaxprio(int ldes1){
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