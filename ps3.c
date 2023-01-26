#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/stat.h>

int simpleShell(FILE *input) {   
    char *line, *token;
    char **tokens;
    size_t bufsize = 4096;
    char *buffer = (char *)malloc(bufsize * sizeof(char));
    char *delimeter = " \r\n";
    ssize_t getLine;
    char *filename;
    int fd_new = -1;
    int mode;
    int position = 0;
    char *PWD;
    struct timeval start, stop;

    if (!(line = buffer)) {
        fprintf(stderr, "Unable to allocate memory for new line: %s\n", strerror(errno));
    }

    if (!(filename = buffer)) {
        fprintf(stderr, "Unable to allocate memory for file path: %s\n", strerror(errno));
    }
    
    tokens = malloc(bufsize * (sizeof(char *)));
    if (tokens == NULL) {
        fprintf(stderr, "Unable to allocate memory for argv: %s\n", strerror(errno));
    }

    while ((getLine = getline(&line, &bufsize, input)) != -1) {
     
        if (line[0] == '#' || line[0] == ' ' || getLine <= 1) {
            continue;
        }

        else
        {
            token = strtok(line, delimeter);

            while (token != NULL) 
            {
                if (token[0] == '<') {
                    mode = O_RDONLY;
                    fd_new++;
                    filename = token + 1;
                }
                else if (token[0] == '>' && token[1] == '>') {
                    mode = O_WRONLY | O_APPEND | O_CREAT;
                    filename = token + 2;
                    fd_new = 1;
                }
                else if (token[0] == '>') {
                    mode = O_WRONLY | O_TRUNC | O_CREAT;
                    filename = token + 1;
                    fd_new = 1;
                }
                else if (token[0] == '2' && token[1] == '>' && token[2] == '>') {
                    mode = O_WRONLY | O_APPEND | O_CREAT;
                    filename = token + 3;
                    fd_new = 2;
                }
                else if (token[0] == '2' && token[1] == '>') {
                    mode = O_WRONLY | O_TRUNC | O_CREAT;
                    filename = token + 2;
                    fd_new = 2;
                }
                else {
                    tokens[position++] = token;
                }
                token = strtok(NULL, delimeter); 
            }
            tokens[position] = NULL;

            if (strcmp(tokens[0], "cd") == 0) {
                if(tokens[1] == NULL) {
                        chdir(getenv("HOME"));
                } else if(chdir(tokens[1])<0) {
                        fprintf(stderr, "Unable to change directory to %s: %s\n", tokens[1], strerror(errno));
                        return -1;
                }
            }
            else if (strcmp(tokens[0], "pwd") == 0) {
                    if (!(PWD = buffer)) {
                        fprintf(stderr, "Unable to allocate memory for directory path: %s\n", strerror(errno));
                        return -1;
                    }
                    // struct stat info;
                    // char buf[BUFSIZE];
                    // lstat(buf, &info);
                    // if ((info.st_mode(S_IFREG))) {
                    //     getcwd(PWD, BUFSIZE);
                    //     printf("%s\n", PWD);
                    //     fprintf(stderr, "%s\n", strerror(errno));
                    //     return -1;
                    // }
                    else {    
                        getcwd(PWD, bufsize);
                        printf("%s\n", PWD);
                    }
            }
            else if (strcmp(tokens[0], "exit") == 0) {
                if (tokens[1] == NULL) {
                    exit(EXIT_FAILURE);  
                }  
                else {
                    exit(atoi(tokens[1])); 
                }
            }

                int fd_old, exit_status;
                int pid = fork();

                struct rusage rusage;
                struct timeval start, stop;

                
                gettimeofday(&start, NULL);
                
                if (pid == -1) {
                    fprintf(stderr, "Unable to fork: %s\n", strerror(errno));
                    exit(-1);
                }
                else if (pid == 0) {
                    if (fd_new > -1)   
                    {
                        if ((fd_old = open(filename, mode, 0666)) != -1) {
                            if (dup2(fd_old, fd_new) == -1) {
                                fprintf(stderr, "Unable to duplicate file for I/O redirection: %s\n", strerror(errno));
                                return -1;
                            } 
                            else if (close(fd_old) == -1) {
                                fprintf(stderr, "Unable to close file for I/O redirection: %s\n", strerror(errno));
                                return -1;
                            }
                        } 
                        else {
                            fprintf(stderr, "Unable to open file %s for I/O redirection: %s\n", filename, strerror(errno));
                            exit(1);
                        }
                    }
                    if (execvp(tokens[0], tokens) == -1) {
                        fprintf(stderr, "Unable to perform exec for %s: %s\n", tokens[0], strerror(errno));
                        exit(127);
                    }
                }
                else {    
                    if (wait(&exit_status) == -1) {
                        fprintf(stderr, "Unable to wait for the child process %s to complete\n", tokens[0]);
                        return -1;
                    }
                
                gettimeofday(&stop, NULL);
                getrusage(RUSAGE_SELF, &rusage);
                
                double time = (stop.tv_sec - start.tv_sec) + ((stop.tv_usec - start.tv_usec)/1000000.0);
                    


                    fprintf(stderr, "Child process %i exited with return value %d\n", getpid(), exit_status);     // Print shell messages
                    fprintf(stderr, "Real: %0.3fs, User: %ld.%03lds, System: %ld.%03lds\n", time, rusage.ru_utime.tv_sec, rusage.ru_utime.tv_usec, rusage.ru_stime.tv_sec, rusage.ru_stime.tv_usec);   
                }
            }
            mode = 0;
            position = 0;
            fd_new = -1;
    }

    free(line);
    free(filename);    
    free(tokens);

    return 0;
}

int main(int argc, char **argv) {
    FILE *input;
    if (argc == 2) {
        if ((input = fopen(argv[1], "r")) == NULL) {
            fprintf(stderr, "Unable to open file %s: %s\n", argv[1], strerror(errno));
            return -1;
        }
        simpleShell(input);  
        
        if (argc == 2 && fclose(input) != 0) {
            fprintf(stderr, "Unable to close file %s: %s\n", argv[1], strerror(errno));
            return -1;
        }
    }

    else 
        simpleShell(stdin);  

    return 0;
}