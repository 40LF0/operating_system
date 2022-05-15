#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

  

// Simple system call

int
my_yield(int num) 
{
	yield();
	return 0xABCDABCD;
}
//Wrapper for my_yield
int
sys_my_yield(void){
	int num;
	if(argint(0,&num)<0)
		return -1;
	return my_yield(num);
}

// how to make a function to syscall
//(the function that is defined in the xv6 
//,but not defined as syscall)

//#1 defined function which name is sys_ + function name you want
int
sys_yield(void){
	return yield();
}
// #2
// add syscall by modifing in file syscall.h syscall.c
// #define SYS_yield 25 //in syscall.h
// extern int sys_yield(void);
// [SYS_yield]   sys_yield,    //in syscall.c
//
// #3
// add function that is used in user stack in file user.h
// int yield(void);
// #4
// add syscall that is used in user stack in file usys.S
//SYSCALL(yield)

// 2022-04-07 wrapper fun for systemcall getlev
int
sys_getlev(void){
  return getlev();
}

// 2022-04-07 wrapper fun for systemcall set_cpu_share
int
sys_set_cpu_share(void){
  int num;
  //Decode argument using argin
  if (argint(0,&num) < 0) 
	return -1; 
  return set_cpu_share(num);

}
// 2022-05-14 wrapper fun for syscall thread_join
int
sys_thread_create(void)
{
  thread_t *thread;
  void*(*start_routine)(void*);
  void *arg;

  if(argptr(0,(char**)&thread, sizeof thread) <0)
	return -1;
  if(argptr(1,(char**)&start_routine, sizeof start_routine) <0)
	return -1;
  if(argptr(2,(char**)&arg, sizeof arg) <0)
	return -1;
  int re;
  re = thread_create(thread,start_routine,arg);
  //cprintf("create su\n");
  return re;

}

// 2022-05-14 wrapper fun for syscall thread_exit
int
sys_thread_exit(void)
{
  void *retval;
  if(argptr(0,(char**)&retval, sizeof retval) <0)
	return -1;
  thread_exit(retval);
  return 0;
}

// 2022-05-14 wrapper fun for syscall thread_join
int
sys_thread_join(void)
{
  int thread;
  void **retval;

  if(argint(0,&thread) <0) 
    return -1; 
  if(argptr(1,(char**)&retval, sizeof retval) <0) 
    return -1; 
  int re;
  re = thread_join((thread_t)thread,retval);
  //cprintf("join su\n");
  return re;

}


int
my_syscall(char *str)
{
	cprintf("%s\n", str);
	return 0xABCDABCD;
}


int
sys_getppid(void)
{
	return myproc() -> parent -> pid;
}


//Wrapper for my_syscall
int
sys_my_syscall(void)
{
	char *str;
	//Decode argument using argstr
	if (argstr(0,&str) <0)
		return -1;
	return my_syscall(str);
}
