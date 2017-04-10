/*Client for VOIP with periodic scheduler and encoding*/
/*Command line arguments: [IP of Server] [Port Address]
/*Press CTRL+C to disconnect the call*/

/*Github Link: https://github.com/nmdesai92/ip-phone*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <time.h>
#include <sys/time.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#include "g711.c"

#define BUFSIZE 4096

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

/*Global variables*/
pa_simple *s = NULL;
int error;

int socket_desc;

/*Continuously send data over Socket*/
static ssize_t loop_send(int fd, const void *data, size_t size)
{
	ssize_t ret  = 0;

	while(size > 0)
	{
		ssize_t r;

		if((r = send(fd, data, size,0)) < 0)
		{
			return r;
		}
		if(r == 0)
		{
			break;
		}

		ret += r;
		data = (const uint8_t *)data + r;
		size -= (size_t) r;
	}
	return ret;
}

/*Timer handler to record and send data*/
void timer_handler(int signum)
{
		uint8_t buf[BUFSIZE];
		int k = 0;

		if(pa_simple_read(s, buf, sizeof (buf), &error) < 0)
		{
			fprintf(stderr,__FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
			if(s) pa_simple_free(s);
			exit(1);
		}
		while(k++ <= BUFSIZE)	buf[k-1] = (uint8_t)linear2ulaw((int)buf[k-1]); //u law encodeing
		/*send data...*/
		if(loop_send(socket_desc, buf, sizeof(buf)) != sizeof(buf))
		{
			perror("send failed:");
		}
}

int main(int argc,char*argv[])
{
	/* The Sample type to use*/
	static const pa_sample_spec ss = {
		.format = PA_SAMPLE_S16LE,
		.rate = 44100,
		.channels = 2
	};

	
	struct sockaddr_in server;

	/*Timer variables*/
	timer_t timerid;
    	struct sigevent sev;
    	struct itimerspec its;
    	struct sigaction sa;

	/*Timer signal setting*/
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = timer_handler;
	sigaction(SIG, &sa, NULL);

	/*Create recording stream*/

	s = pa_simple_new(NULL, argv[0], PA_STREAM_RECORD, NULL, "record", &ss, NULL,NULL, &error);
	if(!s)
	{
		fprintf(stderr, __FILE__" : pa_simple_new() failed : %s\n", pa_strerror(error));
		exit(1);
	}

	/*create socket*/
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_desc == -1)
	{
		printf("Could not create Socket");
	}

	server.sin_addr.s_addr = inet_addr(argv[1]);
	server.sin_family = AF_INET;
	server.sin_port = htons( atoi(argv[2]));

	/*connect to remote server*/

	if((connect(socket_desc, (struct sockaddr *)&server, sizeof server)) < 0)
	{
		perror("bind faild");
		puts("connect error");
		exit(1);
	}

	/*Timer signal parameters*/
	sev.sigev_notify = SIGEV_SIGNAL;
    	sev.sigev_signo = SIG;
    	sev.sigev_value.sival_ptr = &timerid;

        /*Create Timer*/
       if (timer_create(CLOCKID, &sev, &timerid) == -1)
      		   perror("timer_create");

    	/*Set values : Period = 20ms*/
    	its.it_value.tv_sec = 0 ;
    	its.it_value.tv_nsec =  20;
    	its.it_interval.tv_sec = 0;
    	its.it_interval.tv_nsec = 20000000;

    	/*Start Timer*/
    	if (timer_settime(timerid, 0, &its, NULL) == -1)
              perror("timer_settime");

    	while(1);	//Start Scheduling

    	return 0;


}
