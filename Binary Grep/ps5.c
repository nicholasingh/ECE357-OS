#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ctype.h>

int exit_0;
int BUFSIZE = 4096;
int pFlag;
int cLength;
int pOpen;
char *pFile;
jmp_buf env;

void sigbusHandler() {
    fprintf(stderr, "SIGBUS received while processing file");
    longjmp(env, 0);
}

void patternPrint(char *patternStr, int charLen, char *file, int cPrintvalue, int byteOff) {
    exit_0 = 1;
    printf("%s:%d |", file, byteOff);
    int i = 0;
    int j = 0;
    int ret;
    char hex[BUFSIZE];
    if (cPrintvalue == 1) {
        for (i = 0; i < charLen; i++)
            if (isprint(patternStr[i]) !=0) {
                printf("%c", patternStr[i]);
            }
            else 
                printf("?");
    for (i = 0, j = 0; i < charLen; i++, j += 2)
        sprintf(hex + j, "%02x", patternStr[i] & 0xff);
        printf("| %s", hex);
    }
    printf("\n");
    return;
}

int bgrep(int cLength, char *pFile, char **file, int allFiles, int stdinFlag, int cPrintValue) {

    int fd = 0;
    int exit_1 = 0;
    int loop = 0; 
    struct stat fileInfo;
    char *infile;
    int byteOff = 0;
    int fileCnt = 0;
    int stdinRead = 0;

    if (stdinFlag) {
        allFiles = 1;
        fileCnt = 0;
        infile = file[0];
        file[0] = (char *) "stdin";
    }
    exit_0 = 0;

    for (; fileCnt < allFiles; fileCnt++) {

        if(strcmp(file[fileCnt], "stdin")){
            if (lstat(file[fileCnt], &fileInfo) != 0) {
            	fprintf(stderr, "Failed to assign fileInfo on %s: %s\n", file[fileCnt], strerror(errno)); 
            	return -1; 
            }
            fd = open(file[fileCnt], O_RDONLY);
            infile = (char *) mmap(NULL, fileInfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
        }

        if (stdinFlag == 1)
            {
                stdinRead = BUFSIZE/2;
            }
        else {
                stdinRead = fileInfo.st_size;
        }

        for (; byteOff < stdinRead; byteOff++) {
            if (setjmp(env) != 0) {
                exit_1 = -1;
                continue;
            }
            if (!infile[byteOff]) {
                fileCnt++;
                break;
            }
            if (infile[byteOff] == pFile[0]) {
                int cnt = 0;
                int numlooped = 1;
                if (strlen(pFile) == 1) {
                    if (byteOff - cLength - 1 < 0) {
                        patternPrint(&infile[0], (cLength + strlen(pFile)), file[fileCnt], cPrintValue, byteOff);
                        byteOff++;
                        continue;
                    } else {
                        patternPrint(&infile[byteOff - cLength], 2 * cLength + strlen(pFile), file[fileCnt], cPrintValue, byteOff);
                        byteOff++;
                        continue;
                    }
                } else {

                    while (++byteOff < stdinRead && cnt < strlen(pFile) && infile[byteOff] == pFile[++cnt]) {
                        if (setjmp(env) != 0) {
                            exit_1 = -1;
                            fileCnt++;
                            loop = 1;
                            break;
                        }
                        numlooped++;
                        if (numlooped == strlen(pFile) - 1) {
                            if (byteOff - cLength < 1)
                                patternPrint(&infile[0], (2 * cLength + strlen(pFile) + (byteOff - 2 - cLength) - 1), file[fileCnt], cPrintValue, byteOff);
                            else
                                patternPrint(&infile[byteOff - cLength - strlen(pFile) + 2], 2 * cLength + strlen(pFile), file[fileCnt], cPrintValue, byteOff);
                            continue;
                        }
                    }

                    if(loop)
                    	break; 
                }
            }
        }
    }

    if (exit_1 == -1)
        return -1; 
    if (exit_0 != 0)
        return 0;

    return 1;
}

int main(int argc, char **argv) {

    signal(SIGBUS, sigbusHandler);

    pFile = (char *) malloc((sizeof(char)) *BUFSIZE);
    struct stat fileInfo;
    struct stat stdinInfo;
    int redir = 0;
    int stdinFlag = 0;
    int cPrintvalue = 0;

    char ch;
    while ((ch = getopt(argc, argv, "c:p:")) != -1)
        switch (ch) {
            case 'c':
                cLength = atoi(optarg);
                cPrintvalue = 1;
                if (strcmp(optarg, "0") && cLength == 0)
                    fprintf(stderr, "Integer expected after -c\n");
                break;
            case 'p':
                pOpen = open(optarg, O_RDONLY);

                if (lstat(optarg, &fileInfo) != 0)
                    fprintf(stderr, "Failed to get info on argument -p %s: %s\n", optarg, strerror(errno));

                pFile = (char *) mmap(NULL, fileInfo.st_size, PROT_READ, MAP_SHARED, pOpen, 0);
                pFlag = 1;
                break;
            case '?':
                printf("Unknown flag\n");
            default :
                printf("./bgrep {OPTIONS} -p pattern_file {file1 {file2} ...}\n./bgrep {OPTIONS} pattern {file1 {file2} ...}\n");
                exit(-1); 
        }

    char **file = (char **) malloc((sizeof(char)) * (argc - optind) *(BUFSIZE/2));

    if (!pFlag)
        pFile = argv[optind];

    int allFilecnt;
    int index;
    for (allFilecnt = 0; allFilecnt < argc - optind; allFilecnt++) {
        if (!pFlag) {
            pFlag = 2;
            continue;
        }
        if (pFlag == 2)
        index = allFilecnt - 1;
        else {
        index = allFilecnt; 
        }

        file[index] = argv[optind + allFilecnt];
    }

    if ((pFlag == 1 && argc == optind) || (pFlag == 2 && argc - 1 == optind)) {
        redir = 1;
        if (fstat(0, &stdinInfo) != 0)
            fprintf(stderr, "Failed to get info from stdin. Error %d: %s", errno, strerror(errno));
        file[0] = (char *) mmap(NULL, stdinInfo.st_size, PROT_READ, MAP_SHARED, 0, 0);
        stdinFlag = 1;
    }

int allFiles;
int tempallFiles;

    if (pFlag == 2)
        tempallFiles = allFilecnt - 1;
    else {
        tempallFiles = allFilecnt; 
    }
    
    if (redir == 1)
        allFiles = 1;
    else {
        allFiles = tempallFiles;
    }

    return bgrep(cLength, pFile, file, allFiles, stdinFlag, cPrintvalue);
}