#include "fifo.h"

void fifo_init(struct fifo *f){
	cv *readMap = NULL;
	cv *writeMap = NULL;
	readMap = (cv *) mmap (NULL, sizeof(cv), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	writeMap = (cv *) mmap (NULL, sizeof(cv), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(readMap == MAP_FAILED|| writeMap == MAP_FAILED){
        fprintf(stderr,"Failed to mmap ANONYMOUS pages: %s\n",strerror(errno));
        exit(EXIT_FAILURE);
    }
	f->reader = *readMap;
	f->writer = *writeMap;
	cv_init(&f->reader);
	cv_init(&f->writer);
    f->readNext = 0;
	f->writeNext = 0;
    f->full = 0;
    f->fifoLock.p_lock = 0;
}

void fifo_wr(struct fifo *f, unsigned long d){
    spin_lock(&f->fifoLock);

    while (f->full >= MYFIFO_BUFSIZ) {
		 cv_wait(&f->writer, &f->fifoLock); 
	 }

	 f->buf[f->writeNext++] = d;
	 f->writeNext %= MYFIFO_BUFSIZ;
	 f->full++;
    cv_signal(&f->reader);
    spin_unlock(&f->fifoLock);

}

int stream;

unsigned long fifo_rd(struct fifo *f){
	spin_lock(&f->fifoLock);

	while(f->full <= 0) {
        printf("Reader stream %d completed\n",++stream);
        cv_wait(&f->reader, &f->fifoLock);
	}
    
	unsigned long p = f->buf[f->readNext++];
	f->readNext %= MYFIFO_BUFSIZ;
	f->full--;
	cv_signal(&f->writer);
	spin_unlock(&f->fifoLock);
	return p;
}