#include "types.h"
#include "stat.h"
#include "user.h"

#define loops 1000
#define NUM_THREAD 10
 
int buffer;
rwlock_t rwl;

void *writer(void *arg){
  for(int i = 0 ; i < loops ; i ++){
	if(rwlock_acquire_writelock(&rwl) != 0){
	  printf(1,"acquire_writelock error\n");
	  exit();
	}

	buffer++;
    printf(1,"write %d\n",buffer);
	if(rwlock_release_writelock(&rwl) !=0){
	  printf(1,"release_writelock error\n");
	  exit();
	}
	yield();
  }
  thread_exit(0);
  return 0;
}

void *reader(void *arg){
  for(int i = 0 ; i < loops ; i ++){
	if(rwlock_acquire_writelock(&rwl) != 0){
	  printf(1,"acquire_readlock error\n");
	  exit();
	}

	printf(1,"read: %d\n",buffer);

	if(rwlock_release_writelock(&rwl) !=0){
	  printf(1,"release_readlock error\n");
	  exit();
	}
    yield();
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
  buffer = 0;

  if(rwlock_init(&rwl) !=0){
	printf(1,"rwlock_init error\n");
	exit();
  }



  for(int i = 0 ; i < NUM_THREAD/2 ; i++){
	s = thread_create(&t[i*2],reader,(void*)dum);
	if(s != 0){
		printf(1,"thread_create_reader error\n");
		exit();
	}
    s = thread_create(&t[(i*2)+1],writer,(void*)dum);
    if(s != 0){ 
        printf(1,"thread_create_writer error\n");
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

  exit();

}               
