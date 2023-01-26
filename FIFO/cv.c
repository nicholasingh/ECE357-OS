#include "cv.h"

#define BUF_SIZE 4096

void handler(int signum) {
    ;
}

void cv_init(struct cv *cv){
    int *memMap = mmap(NULL, BUF_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0 );
    if(memMap == MAP_FAILED){
       		 fprintf(stderr,"Failed to mmap cv_init: %s\n",strerror(errno));
       		 exit(EXIT_FAILURE);
       	}
    spinlock *lock;
    lock = (spinlock *)(memMap + sizeof(spinlock));
	cv->lock = *lock;
	for(int i = 0; i < CV_MAXPROC; i++){
		cv->pids[i] = 0;
	}
	cv->cnt = 0;

    // if(signal(SIGUSR1, handler) < 0) {
    //         fprintf(stderr,"Error: Failed to handle signal: %s\n",strerror(errno));
    //         exit(EXIT_FAILURE);
    // }

    signal(SIGUSR1, handler);
	sigfillset(&cv->maskNblock);
	sigdelset(&cv->maskNblock, SIGUSR1);
}

void cv_wait(struct cv *cv, struct spinlock *mutex){
	if(cv->cnt >= CV_MAXPROC){
		fprintf(stderr, "Error: Sorry process didn't make the waitlist\n");
		exit(EXIT_FAILURE);
	}

	spin_lock(&cv->lock);
	cv->pids[cv->cnt] = getpid();
	cv->cnt++;
	spin_unlock(&cv->lock);
    spin_unlock(mutex);

	// if(sigprocmask(SIG_BLOCK, &cv->maskNblock, NULL) < 0) {
    //     fprintf(stderr,"Error: Failed to change blocked signals: %s\n", strerror(errno));
    //     exit(EXIT_FAILURE);
    // }

    sigprocmask(SIG_BLOCK, &cv->maskNblock, NULL);

    // if(sigsuspend(&cv->maskNblock) < 0) {
    //     fprintf(stderr,"Error: Failed to wait for signal: %s\n", strerror(errno));
    //     exit(EXIT_FAILURE);
    // }

    sigsuspend(&cv->maskNblock);

    if(cv->cnt > 0){ 
		spin_lock(&cv->lock);
    	cv->pids[cv->cnt-1] = 0; 
    	cv->cnt--;
    	spin_unlock(&cv->lock);
    	spin_lock(mutex); 
    	
        return;
    }

    // if(sigprocmask(SIG_UNBLOCK, &cv->maskNblock, NULL) < 0) {
    //     fprintf(stderr,"Error: Failed to unblock signals: %s\n", strerror(errno));
    //     exit(EXIT_FAILURE);
    // }

    sigprocmask(SIG_UNBLOCK, &cv->maskNblock, NULL);

    spin_lock(mutex);
}

int cv_broadcast(struct cv *cv){
	spin_lock(&cv->lock);
	int wakeCnt = 0;
	if(cv->cnt == 0){
		spin_unlock(&cv->lock);
		return 0;
	}
	for(int i=0; i < CV_MAXPROC; i++){
		if(cv->pids[i] > 0){
			kill(cv->pids[i], SIGUSR1);
			wakeCnt++;
		}
	}
	spin_unlock(&cv->lock);
	return wakeCnt;
}

int cv_signal(struct cv *cv){
	spin_lock(&cv->lock);
	int wakeCnt = 0;

	if(cv->cnt == 0){
		spin_unlock(&cv->lock);
		return 0;
	}

	kill(cv->pids[cv->cnt-1], SIGUSR1);
	wakeCnt++;
	spin_unlock(&cv->lock);

	return wakeCnt;
}