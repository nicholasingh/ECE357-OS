#include "fifo.h"

int process;

int main(int argc, char *argv[]) {

        if(argc != 3) {
            printf("Error: No input after ./spintest\n");
            exit(EXIT_FAILURE);
        }

    long long unsigned int writers = atoll(argv[1]);
	long long unsigned int items = atoll(argv[2]);

	printf ("Beginning test with %llu writers, %llu items each\n", writers, items);

    fifo *f;
	f = (fifo *) mmap (NULL, sizeof (fifo), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(f == MAP_FAILED){
        fprintf(stderr,"Failed to mmap ANONYMOUS page: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
	fifo_init(f);

	pid_t pids[writers];
	for (int i = 0; i < writers; i++) {
		if ((pids[i] = fork()) < 0) {
		        		fprintf (stderr, "Erro: Writer fork failed for fork #%d: %s\n", i, strerror (errno));
		        		return EXIT_FAILURE;
		}
		if (pids[i] == 0) {
			process = i;
			unsigned long bufWrite[items];
			for (int j = 0; j < items; j++) {
                bufWrite[j] = getpid() + j;
				fifo_wr(f, bufWrite[j]);
			}
			fprintf(stderr,"Writer %d completed\n",i);
			exit(EXIT_SUCCESS);
		}
	}

	int reader1;
	if ((reader1 = fork()) < 0) {
		fprintf (stderr, "Error: Reader fork failed: %s\n",strerror (errno));
		return EXIT_FAILURE;
	}
	if (reader1 == 0) {
		process = writers;
		unsigned long bufRead[writers * items];
		int read = writers * items;
		for (int i = 0; i < read; i++) {
			bufRead[i] = fifo_rd(f);
		}
        exit(0);
	}

	for (int i = 0; i < writers+1 ; i++) {
		wait(0);
	}

    printf("All streams done\n");

    return 0;
}