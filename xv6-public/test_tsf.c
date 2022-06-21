#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define loops 1000
#define NUM_THREAD 10


int fd;
thread_safe_guard guard;


void *writer(void *arg){
  int num = (int) arg;
  for(int i = num + 1000*num ; i < loops + 1000*num ; i ++){
    int s = thread_safe_pwrite(&guard,&i,sizeof(int),(num+i)%5*sizeof(int));
	if(s != 0){
		printf(1,"w: %d(%d)\n",i,(num+i)%5);
	}
    yield();
  }
  thread_exit(0);
  return 0;
}

void *reader(void *arg){
  int num = (int)arg;
  int buffer[1];
  for(int i = 0 ; i < loops ; i ++){
    int s = thread_safe_pread(&guard,buffer,sizeof(buffer),num*sizeof(int));
	if(s != 0){
		printf(1,"r: %d(%d)\n",buffer[0],num);
	}
    yield();
  }
  thread_exit(0);
  return 0;
}



int
main(int argc, char *argv[])
{
  fd = open("testfile", O_CREATE|O_RDWR); 
  thread_t t[NUM_THREAD];
  int s;
  void *retval;
  
  guard = *thread_safe_guard_init(fd);

  for(int i = 0 ; i < NUM_THREAD/2 ; i++){
	s = thread_create(&t[i*2],reader, (void*)(i) );
	if(s != 0){
		printf(1,"thread_create_reader error\n");
		exit();
	}
    s = thread_create(&t[(i*2)+1],writer, (void*)(i));
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

  thread_safe_guard_destroy(&guard);

  exit();

}     
