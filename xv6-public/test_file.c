#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

	
#define FILESIZE        (16*1024*1024)  // 16 MB
#define BUFSIZE         512
#define BUF_PER_FILE    ((FILESIZE) / (BUFSIZE))
#define NUM_STRESS      4
#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200



	
int
main(int argc, char *argv[])
{
	int fd, i, j; 
	int r;
	int total;
	char *path = (argc > 1) ? argv[1] : "hugefile";
	char data[BUFSIZE];
	char buf[BUFSIZE];
	
	printf(1, "filetest starting\n");
	const int sz = sizeof(data);
	for (i = 0; i < sz; i++) {
	    data[i] = i % 128;
	}
	
	printf(1, "1. create test\n");
	fd = open(path, O_CREATE | O_RDWR);
	for(i = 0; i < BUF_PER_FILE; i++){
	    if (i % 100 == 0){
	        printf(1, "%d bytes written\n", i * BUFSIZE);
	    }
	    if ((r = write(fd, data, sizeof(data))) != sizeof(data)){
	        printf(1, "write returned %d : failed\n", r);
	        exit();
	    }
	}
	printf(1, "%d bytes written\n", BUF_PER_FILE * BUFSIZE);
	close(fd);
	
	printf(1, "2. read test\n");
	fd = open(path, O_RDONLY);
	for (i = 0; i < BUF_PER_FILE; i++){
	    if (i % 100 == 0){
	            printf(1, "%d bytes read\n", i * BUFSIZE);
	        }
	        if ((r = read(fd, buf, sizeof(data))) != sizeof(data)){
	            printf(1, "read returned %d : failed\n", r);
	            exit();
	        }
	        for (j = 0; j < sz; j++) {
	            if (buf[j] != data[j]) {
	                printf(1, "data inconsistency detected\n");
	                exit();
	            }
	        }
	}
	printf(1, "%d bytes read\n", BUF_PER_FILE * BUFSIZE);
	close(fd);
	
	printf(1, "3. stress test\n");
	total = 0;
	for (i = 0; i < NUM_STRESS; i++) {
	    printf(1, "stress test...%d \n", i);
	    if(unlink(path) < 0){
	        printf(1, "rm: %s failed to delete\n", path);
	        exit();
	    }
	
	    fd = open(path, O_CREATE | O_RDWR);
	    for(j = 0; j < BUF_PER_FILE; j++){
	        if (j % 100 == 0){
	            printf(1, "%d bytes totally written\n", total);
	        }
	        if ((r = write(fd, data, sizeof(data))) != sizeof(data)){
	            printf(1, "write returned %d : failed\n", r);
	            exit();
	        }
	        total += sizeof(data);
	    }
	    printf(1, "%d bytes written\n", total);
	    close(fd);
	}
	
	exit();
}
	
	
	

