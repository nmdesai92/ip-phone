#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#include <pulse/simple.h>
#include <pulse/error.h>

#define BUFSIZE 1024

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

int main(int argc,char*argv[])
{
	/* The Sample type to use*/
	static const pa_sample_spec ss = {
		.format = PA_SAMPLE_S16LE,
		.rate = 44100,
		.channels = 2
	};

	pa_simple *s = NULL;
	int error;

	int socket_desc;
	struct sockaddr_in server;

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

	//puts("connected\n");

	/*record audio*/

	while(1)
	{
		uint8_t buf[BUFSIZE];

		if(pa_simple_read(s, buf, sizeof (buf), &error) < 0)
		{
			fprintf(stderr,__FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
			if(s) pa_simple_free(s);
			exit(1);
		}
		//write(STDOUT_FILENO, buf,sizeof (buf));
		sleep (0.5);
		if(loop_send(socket_desc, buf, sizeof(buf)) != sizeof(buf))
		{
			perror("send failed:");
		}
	}

	if (s)
        pa_simple_free(s);

    return 0;


}