#include "types.h"
#include "stat.h"
#include "user.h"
 
int
main(int argc, char *argv[])
{
	int ret_val;
	ret_val = getpid();
	printf(1,"My pid is %x\n", ret_val);
	ret_val = getppid();
	printf(1,"My ppid is %x\n", ret_val);

    exit();
}          
