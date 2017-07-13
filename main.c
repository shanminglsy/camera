#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <malloc.h>
#include "video_capture.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include<sys/stat.h>

#define MYPORT  6632
#define BUFFER_SIZE 1024

struct camera *cam;
pthread_t mythread;
pthread_t sendthread;

void capture_encode_thread(void) {
	int count = 1;
	for (;;) {
		printf("\n\n-->this is the %dth frame\n", count);
		if (count++ >= 30) 
				{
			printf("------need to exit from thread------- \n");
		//	v4l2_close(cam);
			break;
		}

		fd_set fds;
		struct timeval tv;
		int r;

		FD_ZERO(&fds);
		FD_SET(cam->fd, &fds);

		/* Timeout. */
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		r = select(cam->fd + 1, &fds, NULL, NULL, &tv);

		if (-1 == r) {
			if (EINTR == errno)
				continue;

			errno_exit("select");
		}

		if (0 == r) {
			fprintf(stderr, "select timeout\n");
			exit(EXIT_FAILURE);
		}

		if (read_and_encode_frame(cam) != 1) {
			fprintf(stderr, "read_fram fail in thread\n");
			break;
		}
	}
}

void send_h264_thread(void)
{
   printf("come in send thread");
    int sock_cli = socket(AF_INET,SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(MYPORT);
    servaddr.sin_addr.s_addr = inet_addr("192.168.10.228");

    if (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect");
        exit(1);
    }

    char sendbuf[BUFFER_SIZE];
    char recvbuf[BUFFER_SIZE];
 //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    char buffer[BUFFER_SIZE];
    int  file_block_length = 0;
    int  count;
    bzero(buffer,BUFFER_SIZE);
    strcpy(sendbuf,"begin");
    //strcat(sendbuf,"\n");
    send(sock_cli,sendbuf,strlen(sendbuf),0);
    bzero(sendbuf,sizeof(sendbuf));
    while (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
    {
	 FILE *fp = fopen("a.bmp","r");
   	 if (fp == NULL )
    	{
        	printf("File is not found");

    	}
	else
	{
        	while( (file_block_length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0)
        	{
//            		printf("file_block_length = %d", file_block_length);
              		printf("  _ count  %d\n",++count);
              		if (send(sock_cli, buffer, file_block_length, 0) < 0)
             		{
                   		printf("Send File:Failed!\n");
                  		break;
             		}

              		bzero(buffer, sizeof(buffer));
        	}
        	fclose(fp);
     		 printf("File Transfer Finished!\n");
    	}
    }
     	printf("socket close");
     	close(sock_cli);
   
}



int main(int argc, char **argv) {
	cam = (struct camera *) malloc(sizeof(struct camera));
	if (!cam) {
		printf("malloc camera failure!\n");
		exit(1);
	}
	cam->device_name = "/dev/video1";
	cam->buffers = NULL;
	cam->width = 640;
	cam->height = 480;
	cam->display_depth = 5; /* RGB24 */

	v4l2_init(cam);

	if (0 != pthread_create(&mythread, NULL, (void *) capture_encode_thread, NULL)) {
		fprintf(stderr, "thread create fail\n");
	}

	if (0 != pthread_create(&sendthread,NULL,(void *) send_h264_thread,NULL))
       {
	  printf("hello,send_thread_error");
	}
	pthread_join(mythread, NULL);
	pthread_join(sendthread,NULL);
	printf("-----------end program------------");
	v4l2_close(cam);

	return 0;
}
