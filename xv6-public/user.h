struct stat;
struct rtcdate;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int my_syscall(char*);
int getppid(void);
int my_yield(int);
int yield(void);
int getlev(void);
int set_cpu_share(int);
int thread_create(thread_t*,void*(*)(void*),void*);
void thread_exit(void*);
int thread_join(thread_t,void**);
int xem_init(xem_t*);
int TAS(int*,int);
int xem_wait(xem_t*);
int xem_unlock(xem_t*);
int	rwlock_init(rwlock_t*);
int	rwlock_acquire_readlock(rwlock_t*);
int rwlock_release_readlock(rwlock_t*);
int rwlock_acquire_writelock(rwlock_t*);
int rwlock_release_writelock(rwlock_t*);
int pwrite(int,void*,int,int);
int pread(int,void*,int,int);




// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);

//thread_safe_guard.c
thread_safe_guard* thread_safe_guard_init(int fd);
int thread_safe_pread(thread_safe_guard* file_guard, void* addr, int n, int off);
int thread_safe_pwrite(thread_safe_guard* file_guard, void* addr, int n, int     off);
void thread_safe_guard_destroy(thread_safe_guard *file_guard);
