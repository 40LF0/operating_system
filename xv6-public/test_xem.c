#include "types.h"
#include "stat.h"
#include "user.h"

#define loops 10000 
#define NUM_THREAD 10
 
int shared = 0;
xem_t xem;


void *threadFunc(void* arg){
  for(int j = 0 ; j < loops ; j ++){
	if(xem_wait(&xem) != 0){
	  printf(1,"xem_wait error\n");
	  exit();
	}

	shared++;

	if(xem_unlock(&xem) !=0){
	  printf(1,"xem_unlocl error\n");
	  exit();
	}

  }
  thread_exit(0);
  return 0;

  
}

int
main(int argc, char *argv[])
{
  thread_t t[NUM_THREAD];
  int s;
  int dum = 0;
  void *retval;

  if(xem_init(&xem) !=0){
	printf(1,"sem_init error\n");
	exit();
  }


  for(int i = 0 ; i < NUM_THREAD ; i++){
	s = thread_create(&t[i],threadFunc,(void*)dum);
	if(s != 0){
		printf(1,"thread_create error\n");
		exit();
	}
  }
  for(int i = 0 ; i < NUM_THREAD ; i++){
    s = thread_join(t[i],&retval);
    if(s != 0){ 
        printf(1,"thread_join error\n");
        exit();
    }   
  }

  printf(1,"shared = %d\n",shared);
  exit();

}               
