#define DELETED -6
#define READ 	1
#define WRITE	2
#define NLOCKS	50

#ifndef NPROC
#define NPROC	50
#endif


//lock states
#define UNUSED 		0
#define ACQUIRED 	1
#define AVAILABLE	2

#define	isbadlock(l)	(l<0 || l>=NLOCKS)


//lock data structure
struct	lock	{
	int state;	//state of the lock
	int head;	//head of the semaphore quene
	int tail;	//tail of the semaphore queue
	int type; 	//type of the process that acquired the lock
	int pacq[NPROC];	//processes that have acquired the lock
	int readers; //number of readers
};

extern int numofreaders;

extern int nextlock;

extern struct lock locktab[];

/*
void linit();
int lcreate();
int ldelete(int lockdescriptor);
int lock(int ldes1, int type, int priority);
int releaseall(int numlocks, int ldes1, ...);
*/