#include <stdio.h>
#include <kernel.h>
#include <sem.h>
#include <lock.h>
#include <proc.h>

extern int ctr1000;
extern int currpid;
extern struct pentry proctab[];
int proceed = 0;

void lproclock(int lck){
	kprintf("%s begins\n", proctab[currpid].pname);
	kprintf("%s priority: %d\n", proctab[currpid].pname, proctab[currpid].pprio);

	//tries to acquire lock
	kprintf("%s trying to acquire lock\n", proctab[currpid].pname);
	int t1 = ctr1000;
	int res = lock(lck, WRITE, 20);
	int t2 = ctr1000;
	if(res == OK)
		kprintf("%s acquired the lock successfully\n", proctab[currpid].pname);
	else
		kprintf("%s is unable to acquire the lock\n", proctab[currpid].pname);

	sleep(5);
	//do some computation
	int i = 1;
	while(i){
		i++;
		if(i == 1000000)
			break;
	}

	kprintf("%s priority: %d\n", proctab[currpid].pname, proctab[currpid].pprio);
	//release the lock
	kprintf("%s releases the lock\n", proctab[currpid].pname);
	releaseall(1, lck);
	kprintf("%s priority: %d\n", proctab[currpid].pname, proctab[currpid].pprio);
	kprintf("%s waited %d millisec for the lock\n", proctab[currpid].pname, (t2-t1)*10);
	kprintf("%s finishes\n", proctab[currpid].pname);

}

void lprocnolock(){
	kprintf("%s begins\n", proctab[currpid].pname);
	kprintf("%s priority: %d\n", proctab[currpid].pname, proctab[currpid].pprio);

	sleep(3);
	//do some computation
	int i = 1;
	while(i){
		i++;
		if(i == 1000000000)
			break;
	}

	kprintf("%s finishes\n", proctab[currpid].pname);

}

void sprocwait(int sem){
	kprintf("%s begins\n", proctab[currpid].pname);
	kprintf("%s priority: %d\n", proctab[currpid].pname, proctab[currpid].pprio);

	//tries to acquire semaphore
	kprintf("%s trying to acquire semaphore\n", proctab[currpid].pname);
	int t1 = ctr1000;
	wait(sem);
	int t2 = ctr1000;
	kprintf("%s acquired the semaphore successfully\n", proctab[currpid].pname);

	sleep(5);
	//do some computation
	int i = 1;
	while(i){
		i++;
		if(i == 1000000)
			break;
	}

	kprintf("%s priority: %d\n", proctab[currpid].pname, proctab[currpid].pprio);
	//release the semaphore
	kprintf("%s releases the semaphore\n", proctab[currpid].pname);
	signal(sem);
	kprintf("%s priority: %d\n", proctab[currpid].pname, proctab[currpid].pprio);
	kprintf("%s waited %d millisec for the semaphore\n", proctab[currpid].pname, (t2-t1)*10);
	kprintf("%s finishes\n", proctab[currpid].pname);
	proceed++;
}

void sprocnowait(int sem){
	kprintf("%s begins\n", proctab[currpid].pname);
	kprintf("%s priority: %d\n", proctab[currpid].pname, proctab[currpid].pprio);

	sleep(3);
	//do some computation
	int i = 1;
	while(i){
		i++;
		if(i == 1000000000)
			break;
	}

	kprintf("%s finishes\n", proctab[currpid].pname);
	proceed++;
}

void task1(){
	//test semaphores for priority inheritance
	kprintf("\nXINU SEMAPHORES:\n");
	int sem = screate(1);
	kprintf("Semaphore created successfully\n");
	int p1 = create(sprocwait, 2000, 20, "P1", 1, sem);
	int p2 = create(sprocnowait, 2000, 25, "P2", 1, sem);
	int p3 = create(sprocwait, 2000, 30, "P3", 1, sem);

	resume(p1);
	sleep(2);

	resume(p2);
	sleep(2);

	resume(p3);
	sleep(2);

	while(1){
		if(proceed == 3)
			break;
	}

	//test locks for priority inheritance
	kprintf("\nPA2 LOCKS:\n");
	int lck = lcreate();

	int p4 = create(lproclock, 2000, 20, "P1", 1, lck);
	int p5 = create(lprocnolock, 2000, 25, "P2", 1, lck);
	int p6 = create(lproclock, 2000, 30, "P3", 1, lck);

	
	resume(p4);
	sleep(2);

	resume(p5);
	sleep(2);

	resume(p6);
	
}