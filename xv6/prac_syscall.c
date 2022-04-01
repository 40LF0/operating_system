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
	int num;
    num = yield();
     if(argint(0,&num)!=0)
          return -1;
     return 0;
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
