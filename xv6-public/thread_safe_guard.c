#include "types.h"
#include "stat.h"
#include "user.h"



thread_safe_guard* 
thread_safe_guard_init
(int fd){
	thread_safe_guard* safe_guard;
	safe_guard = malloc(sizeof(thread_safe_guard));
	safe_guard->fd = fd;
	rwlock_init(&safe_guard->rw);
	return safe_guard;
}


int 
thread_safe_pread
(thread_safe_guard* file_guard, void* addr, int n, int off){
	int p;
	rwlock_acquire_readlock(&file_guard->rw);
	p = pread(file_guard->fd,addr,n,off);
	rwlock_release_readlock(&file_guard->rw);
	return p;
}

int 
thread_safe_pwrite
(thread_safe_guard* file_guard, void* addr, int n, int off){
	int p;
    rwlock_acquire_writelock(&file_guard->rw);
    p = pwrite(file_guard->fd,addr,n,off);
    rwlock_release_writelock(&file_guard->rw);
    return p;
}   
void thread_safe_guard_destroy(thread_safe_guard *file_guard){
	free(file_guard);
}

