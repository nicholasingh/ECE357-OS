#include <sched.h>
#include "tas.h"
#include "spinlock.h"

void spin_lock(struct spinlock *l){
	while(tas(&(l->p_lock)) != 0){
		sched_yield();
	}
}

void spin_unlock(struct spinlock *l){
	l->p_lock = 0;
}