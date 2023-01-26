#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include "spinlock.h"

int main(int argc, char *argv[]) {

        if(argc != 3) {
                printf("Error: No input after ./spintest\n");
                exit(EXIT_FAILURE);
        }

        long long unsigned int processes = atoll(argv[1]);
        long long unsigned int iterations = atoll(argv[2]);

        printf ("Number of Processes = %llu\n", processes);
        printf ("Number of Iterations = %llu\n", iterations);

        int * memMap1 = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0 );
        int * memMap2 = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0 );

        if(memMap1 == MAP_FAILED || memMap2 == MAP_FAILED) {
                fprintf(stderr,"Error: Failed to mmap ANONYMOUS pages: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

        memMap1[0] = 0;
        memMap2[0] = 0;

        spinlock *lock;
        lock = (spinlock *)(memMap1 + sizeof(spinlock));
        lock->p_lock = memMap1[1];

        pid_t pids[processes];

        for (int i = 0; i < processes; i++) {
                if ((pids[i] = fork()) < 0) {
                        fprintf (stderr, "Error: Failed to fork for Process %d: %s\n", i, strerror(errno));
                        return EXIT_FAILURE;
                }
                if (pids[i] == 0) {
                        for (int j = 0; j < iterations; j++) {
                                memMap2[0]++;
                        }
                        spin_lock(lock);

                        for (int k = 0; k < iterations; k++) {
                                memMap1[0]++;
                        }
                        spin_unlock(lock);

                        exit(0);
                }
        }

        for (int m = 0; m < processes; m++) {
                if (waitpid(pids[m], NULL, 0) < 0) {
                        fprintf (stderr, "Error: wait system call failed: %s\n", strerror (errno));
                }
        }

        printf("Mutex Protection = %d\n", memMap1[0]);
        printf("No Mutex Protection = %d\n", memMap2[0]);
}