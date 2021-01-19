/* ldelete.c - ldelete */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * ldelete  --  delete a lock by releasing its table entry
 *------------------------------------------------------------------------
 */
SYSCALL ldelete(int lock)
{
	STATWORD ps;    
	int	pid;
	struct 	lock	*lptr;

	disable(ps);
	if (isbadlock(lock) || locktab[lock].state==EMPTY) {
		restore(ps);
		return(SYSERR);
	}
	lptr = &locktab[lock];
	lptr->state = UNUSED;
	lptr->type = UNUSED;
	lptr->readers = 0;
	int p;
	for(p = 0; p<NPROC; p++){
		locktab[lock].pacq[p] = UNUSED;
		if(p != 49 && proctab[p].pstate != PRFREE){
			proctab[p].ldeleted = 1;
		}
	}
	if (nonempty(lptr->head)) {
		while( (pid=getfirst(lptr->head)) != EMPTY)
		  {
		    proctab[pid].pwaitret = DELETED;
		    ready(pid,RESCHNO);
		  }
		resched();
	}
	restore(ps);
	return(OK);
}
