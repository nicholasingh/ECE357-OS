#include "tas.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct spinlock {
	volatile char p_lock;
}
spinlock;

void spin_lock(struct spinlock *l);
void spin_unlock(struct spinlock *l);