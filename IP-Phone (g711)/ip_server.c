/*Srrver for VOIP with periodic scheduler and decoding*/
/*Port number for this particular server is 5000*/

/*Github Link: https://github.com/nmdesai92/ip-phone*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <time.h>
#include <sys/time.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#include "g711.c"

#define PORT "5000"
#define BUFSIZE 4096


#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

/*Global variables*/
pa_simple *s = NULL;
int error,new_socket;

uint8_t buf[BUFSIZE];
ssize_t r;

/*Timer handler to receive and play*/
static void timer_handler (int signum)
{
	int k = 0;
	/* receive audio data...*/
	if((r = recv(new_socket, buf, sizeof(buf), 0)) <= 0)
	{
		if(r == 0)	/*Disconnect call if client is off*/
		{
				printf("call disconnected\n");
				exit(0);

		}
		perror("read() failed :");
		if(s)	pa_simple_free(s);
		exit(1);
	}

	while(k++ <= BUFSIZE)	buf[k-1] = (uint8_t)ulaw2linear((int)buf[k-1]);	//ulaw to linear decoding
	/*play audio...*/
	 if (pa_simple_write(s, buf, (size_t) r, &error) < 0) {
		perror("pa_simple_write() failed:");
	 
	 	if(s)	pa_simple_free(s);
     }

    	/*Make sure that every single sample is played*/
	if(pa_simple_drain(s, &error) < 0)
	{
		perror("pa_simple_drain() failed :");
		if(s)	pa_simple_free(s);
		exit(1);
	}	
}

int main(int argc,char **argv)
{
	/*The sample format to use*/
	static const pa_sample_spec ss = {
		.format = PA_SAMPLE_S16LE,
		.rate = 44100,
		.channels = 2
	};
	
	/*Socket variables*/
	int socket_desc, c;
	struct addrinfo hints, *server, *p;
	struct sockaddr_in client;
	int rv;

	/*Timer variables*/
	timer_t timerid;
      	struct sigevent sev;
      	struct itimerspec its;
      	struct sigaction sa;

      	/* Timer interrupt handler declaration*/
      	memset (&sa, 0, sizeof (sa));
      	sa.sa_handler = timer_handler;
      	sigaction(SIG, &sa, NULL);

	/*Socket parameters setting*/
	memset(&hints, 0,sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	/*Create a new Playback stream*/
	s = pa_simple_new(NULL, argv[0],PA_STREAM_PLAYBACK,NULL,"Playback", &ss,NULL,NULL,&error);
	if(!s)
	{
		perror("pa_simple_new failed()");
		if(s)	pa_simple_free(s);
		exit(1);
	}

	/*get details of server*/
	if((rv = getaddrinfo(NULL, PORT, &hints, &server)) != 0)
	{
		perror("getaddrinfo() :");
		exit(1);
	}

	p = server;

	/*create Socket*/
	socket_desc = socket(p->ai_family,p->ai_socktype,p->ai_protocol);
	if(socket_desc == -1)
	{
		printf("could not create socket");
	}

	/*Bind to socket*/
	if(bind(socket_desc,p->ai_addr,p->ai_addrlen) < 0)
	{
		close(socket_desc);
		perror("bind failed");
		exit(1);
	}

	puts("bind done");

	freeaddrinfo(server);
	
	/*Listen*/
	listen(socket_desc, 10);

	puts("Waiting for incoming calls...");

	c = sizeof(struct sockaddr_in);

	/*accept the connection*/
	new_socket = accept(socket_desc,(struct sockaddr *)&client,(socklen_t *)&c);
	if(new_socket == -1)
	{
		perror("accept");
	}
	puts("connection accepted");

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIG;
	sev.sigev_value.sival_ptr = &timerid;
	  
	 /*Create timer*/ 
	if (timer_create(CLOCKID, &sev, &timerid) == -1)
	     perror("timer_create");

	 /*Set values : period = 1 us*/
    	its.it_value.tv_sec = 0 ;
    	its.it_value.tv_nsec =  20;
    	its.it_interval.tv_sec = 0;
    	its.it_interval.tv_nsec = 10000;		//10 us

    	/*Start the 20ms timer*/
    	if (timer_settime(timerid, 0, &its, NULL) == -1)
              perror("timer_settime");  
    
    	while(1);	//Start scheduling
   
   	return 0;
}
