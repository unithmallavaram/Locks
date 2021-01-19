
#include <kernel.h>
#include <stdio.h>
#include <lock.h>
#include <q.h>

struct lock locktab[NLOCKS];
int numberofreaders;
int nextlock;

SYSCALL linit(){
	STATWORD ps;
	disable(ps);

	//initialize the lock table
	int i,j;
	for(i = 0; i<NLOCKS; i++){
		locktab[i].state = UNUSED;
		locktab[i].type = UNUSED;
		locktab[i].head = newqueue();
		locktab[i].tail = locktab[i].head + 1;
		for(j = 0; j<NPROC; j++){
			locktab[i].pacq[j] = UNUSED;
		}
		locktab[i].readers = 0;
	}

	//
	numberofreaders = 0;
	nextlock = NLOCKS - 1;

	restore(ps);
	return(OK);
}