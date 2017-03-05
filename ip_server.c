#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

#include <pulse/simple.h>
#include <pulse/error.h>

#define PORT "8888"
#define BUFSIZE 100

int main(int argc,char **argv)
{
	/*The sample format to use*/
	static const pa_sample_spec ss = {
		.format = PA_SAMPLE_S16LE,
		.rate = 44100,
		.channels = 2
	};

	pa_simple *s = NULL;
	int error;

	int socket_desc, new_socket, c;
	struct addrinfo hints, *server, *p;
	struct sockaddr_in client;
	int rv;

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


	while(1)
	{
		uint8_t buf[BUFSIZE];
		ssize_t r;
		
		/* receive audio data...*/
		if((r = recv(new_socket, buf, sizeof(buf), 0)) <= 0)
		{
			if(r == 0)
			{
					printf("call disconnected\n");
					//flag = 1;
					break;

			}
			perror("read() failed :");
			if(s)	pa_simple_free(s);
			exit(1);
		}
		
		/*play audio...*/
		 if (pa_simple_write(s, buf, (size_t) r, &error) < 0) {
        	perror("pa_simple_write() failed:");
        	if(s)	pa_simple_free(s);
    	}
    	
	}

	/*Make sure that every single sample is played*/
	if(pa_simple_drain(s, &error) < 0)
	{
		perror("pa_simple_drain() failed :")
		if(s)	pa_simple_free(s);
		exit(1);
	}	

	if (s)
        pa_simple_free(s);

	return 0;
}