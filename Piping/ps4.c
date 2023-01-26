#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <setjmp.h>
//#define BUFSIZE 4096  

void sigusr1_handler(int signum);
void sigusr2_handler(int signum);
void pipesCloseHand();

int BUFSIZE = 4096;
int bytes, files;
int fdGrep[2], fdMore[2];
jmp_buf int_jb;
struct sigaction sa;
struct sigaction sa2;

void sigusr1_handler(int signum) {
    fprintf(stderr,"Total files processed: %d, Total bytes processed: %d\n", files, bytes);
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sigusr1_handler;
    sa.sa_flags = 0;
}

void sigusr2_handler(int signum) {
    fprintf(stderr,"*** SIGUSR2 received, moving on to the next file\n");
    longjmp(int_jb, 1);
}

void pipesCloseHand() {
    if(close(fdGrep[0]) < 0) {
        fprintf(stderr, "Error while closing read end of grep: %s\n", strerror(errno));
        exit(1);
    }
    if(close(fdGrep[1]) < 0){
        fprintf(stderr, "Error while closing write end of grep: %s\n", strerror(errno));
        exit(1);
    }
    if(close(fdMore[0]) < 0) {
        fprintf(stderr, "Error while closing read end of more: %s\n", strerror(errno));
        exit(1);
    }
    if(close(fdMore[1]) < 0) {
        fprintf(stderr, "Error while closing write end of more: %s\n", strerror(errno));
        exit(1);
    }
}

int main(int argc, char *argv[]) {

    int statusGrep, statusMore;
    int fdIn, bytesRead, bytesWritten;
    sigset_t old_set;

    sigemptyset(&old_set);

	char *buf =  malloc((sizeof(char)) *BUFSIZE);

    // struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sa.sa_flags = 0;

    // struct sigaction sa2;
    sa2.sa_handler = sigusr2_handler;
    sa2.sa_flags = 0;

    sigemptyset(&sa.sa_mask);

    if(sigaction(SIGUSR1, &sa, 0) < 0) {
        fprintf(stderr, "Failed to handle user defined signal 1:\n %s\n", strerror(errno));
    }

    sigemptyset(&sa2.sa_mask);

    if(sigaction(SIGUSR2, &sa2, 0) < 0) {
        fprintf(stderr, "Failed to handle user defined signal 2:\n %s\n", strerror(errno));
    }

    if(argc < 3) {
        fprintf(stderr,"Not enough arguments\nCorrect syntax: ./ps4 pattern infile1 [...infile2...]\n");
        exit(1);
    }

    for(int i = 2; i < argc; i++) {

        if (setjmp(int_jb) != 0) {
            i++;
            if(sigaddset(&old_set, SIGUSR2)==0)
                {
                    sigprocmask(SIG_UNBLOCK,&old_set,NULL);
                }
        }

        if((fdIn = open(argv[i], O_RDONLY)) < 0) {
            fprintf(stderr, "Error opening input file %s: %s\n", argv[i], strerror(errno));
            exit(1);
        }

        files++;

        if (pipe(fdGrep) < 0) {
            fprintf(stderr, "Error making grep pipe: %s\n", strerror(errno));
            exit(1);
        }
        if (pipe(fdMore) < 0) {
            fprintf(stderr, "Error making more pipe: %s\n", strerror(errno));
            exit(1);
        }

        int pidGrep = fork();

        switch(pidGrep) {
            case -1:
                fprintf(stderr, "Error forking grep process: %s\n", strerror(errno));
                exit(1);
                break;
                
            case 0:
                if(dup2(fdGrep[0], 0) < 0) {
                    fprintf(stderr, "Error duping read end of grep to STDIN: %s\n", strerror(errno));
                    exit(1);
                }
                if(dup2(fdMore[1], 1) < 0) {
                    fprintf(stderr, "Error duping write end of more to STDOUT: %s\n", strerror(errno));
                    exit(1);
                }

                pipesCloseHand();   
                if(close(fdIn) < 0) {
                    fprintf(stderr, "Error closing input file %s: %s\n", argv[i], strerror(errno));
                    exit(1);
                }

                if (execlp("grep","grep", argv[1], NULL) < 0) { 
                    fprintf(stderr, "Error executing grep command: %s\n", strerror(errno));
                    exit(1);
                }
                break;
        }
        
        int pidMore = fork();

        switch(pidMore) {
            case -1:
                fprintf(stderr, "Error forking more process: %s\n", strerror(errno));
                exit(1);
                break;
            
            case 0:
                if (dup2(fdMore[0], 0) < 0) {
                    fprintf(stderr, "Error duping read end of more pipe to STDIN: %s\n", strerror(errno));
                    exit(1);
                }

                pipesCloseHand();
                if(close(fdIn) < 0) {
                    fprintf(stderr, "Error closing input file %s: %s\n", argv[i], strerror(errno));
                    exit(1);
                }

                if (execlp("more", "more", NULL) < 0) {
                    fprintf(stderr, "Error executing more command: %s\n", strerror(errno));
                    exit(1);
                }
                break;
        }

        while ((bytesRead = read(fdIn, buf, BUFSIZE)) > 0) {
            if (bytesRead < 0) {  
				if (errno == EINTR)
				{
                    fprintf(stderr, "Error reading input file %s: %s\n", argv[i], strerror(errno));
					continue;
				}
				buf += bytesRead;
                BUFSIZE -= bytesRead;
            }

            else {
                bytesWritten = write(fdGrep[1], buf, bytesRead);
                if (bytesWritten < 0) { 
					if (errno == EINTR)
					{
						continue;
                        fprintf(stderr, "Error while writing to grep pipe: %s\n", strerror(errno)); 
					}
					buf += bytesWritten;
                    bytesRead -= bytesWritten;
                } 
                
                else if (bytesWritten < bytesRead) {
                    fprintf(stderr, "Partial write occuring within input file %s: %s\n", argv[i], strerror(errno));
                    bytesRead = bytesRead - bytesWritten;
                    buf = buf + bytesRead;
                    bytes += bytesWritten;
                    bytesWritten = 0;
                } 
                else {
                    bytes += bytesWritten;
                }
            }
        }

        if(close(fdIn) < 0) {
            fprintf(stderr, "Error: Failed to close input file %s: %s\n", argv[i], strerror(errno));
            exit(1);
        }
        pipesCloseHand();

        waitpid(pidGrep, &statusGrep, 0); 
        waitpid(pidMore, &statusMore, 0);
    }

    return 0; 
}