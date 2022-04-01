#include "types.h"
#include "stat.h"
#include "user.h"
 
int
main(int argc, char *argv[])
{
	int p_pid = getpid();
	fork();
	
	for(int i = 0 ;i<10 ; i++){
		yield();
    //    printf(1,"%d\n",yield());
		if (getpid() == p_pid)
			printf(1,"P\n");
		else
			printf(1,"C\n"); 
		
	}
	
    exit();
}               
