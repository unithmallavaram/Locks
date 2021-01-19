/* lcreate.c - lcreate, newlock */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

LOCAL int newlock();

/*------------------------------------------------------------------------
 * lcreate  --  create and initialize a lock, returning its id
 *------------------------------------------------------------------------
 */
SYSCALL lcreate()
{
	STATWORD ps;    
	int	lock;

	disable(ps);
	if ( (lock=newlock())==SYSERR ) {
		restore(ps);
		return(SYSERR);
	}
	//locktab[lock].semcnt = count;
	/* head and tail were initialized at system startup */
	restore(ps);
	return(lock);
}

/*------------------------------------------------------------------------
 * newlock  --  allocate an unused lock and return its index
 *------------------------------------------------------------------------
 */
LOCAL int newlock()
{
	int	lock;
	int	i;

	for (i=0 ; i<NLOCKS ; i++) {
		lock=nextlock--;
		if (nextlock < 0)
			nextlock = NLOCKS-1;
		if (locktab[lock].state==UNUSED) {
			locktab[lock].state = AVAILABLE;
			return(lock);
		}
	}
	return(SYSERR);
}
