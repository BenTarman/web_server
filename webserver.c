#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>

#define KRED   "\x1b[31m"
#define KYEL   "\x1b[33m"
#define KGREEN "\x1b[32m"
#define KBLUE  "\x1b[34m"
#define KRESET "\x1b[0m"

#include <signal.h>

static volatile int keepRunning = 1;
	
int isVerbose = 1; //global variable for  verbose mode

int lfd = -1;


void debug(char* format, ...) 
{
	if (isVerbose) 
	{
		va_list args;
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
	}
}

void closeHandler(int whocares) {
	debug(KRED "Closing server" KRESET "\n");
	keepRunning = 0;
	exit(0);
}


void start_webserver()
{
	struct sockaddr_in server;
	
	if ((lfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) 
	{
		debug( KRED "Failed to create listening socket" KRESET "\n");
		exit(-1);
	}

	int port = atoi("6001");

	memset(&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = PF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);

	
	if (bind(lfd, (struct sockaddr *) &server, 
				sizeof(struct sockaddr_in)) == -1) 
	{
		debug(KRED "bind socket failed %s" KRESET "\n", strerror(errno));
	}

	debug(KGREEN "listening on port %d" KRESET "\n", port);
	
	if (listen(lfd, 10000) == -1) 
	{
		debug( KRED "listen failed %s" KRESET "\n", strerror(errno));
	}


}



void respond_to_client(int connfd)
{
	char buffer[99999]; 
	ssize_t read_size;

	int bytes_read;
	char data[512];

	char* path = getenv("PWD");

	printf("file: %s\n", path);

	memset( (void*)buffer, (int)'\0', 99999 );
	if ((read_size = recv(connfd, buffer, 99999, 0)) > 0)
	{
		printf("%s\n", buffer);
		char *req = strtok(buffer, "\n");
		char *req_type = malloc(6);
		char *file_to_serve = malloc(40);
		char *protocol_version = malloc(10);
		sscanf(req, "%s%s%s", req_type, file_to_serve, protocol_version);


		char* pos = strstr(file_to_serve, "."); //IP: the original string
		char fileName[40];
		memcpy(fileName, file_to_serve, pos - file_to_serve);
	
		if ((strcmp(fileName, "/favicon") != 0) &&
				strchr("1234567890", fileName[strlen(fileName) - 1]) == NULL)
		{
				write(connfd, "HTTP/1.0 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n",224);
		}

		strcat(path, file_to_serve);

		if (strcmp(protocol_version, "HTTP/1.0") != 0 &&
				strcmp(protocol_version, "HTTP/1.1") != 0)
		{
				write(connfd, "HTTP/1.0 400 Bad Request\n", 25);
		}

		if (strcmp(req_type, "GET") == 0)
		{
			int fd = open(path, O_RDONLY);

			if (fd == -1)
			{
				write(connfd, "HTTP/1.0 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n",224);
			}
			else
			{
				write(connfd, "HTTP/1.0 200 OK\n\n", 17);
				while ((bytes_read = read(fd, data, 512))>0 )
				{
					write (connfd, data, bytes_read);
				}
			}
		}
		else
		{
			// requests types aren't implemented so say its a bad request
			write(connfd, "HTTP/1.0 400 Bad Request\n", 25);
		}
	}

	shutdown (connfd, SHUT_RDWR);
	close(connfd);

}

void connect_webserver()
{
	struct sockaddr_in clientaddr;
	int connfd = -1;

	while (keepRunning)
	{
		socklen_t addrlen = sizeof(clientaddr);

		debug( KYEL "accepting socket...." KRESET "\n");
		if ((connfd = accept(lfd, 
						(struct sockaddr *) &clientaddr, &addrlen)) < 0) 
		{
			debug( KRED "accept failed %s" KRESET "\n", strerror(errno));
			exit(-1);
		}

		if (fork() == 0)
		{
			respond_to_client(connfd);
			//close(connfd);
			exit(0);
		}

	}

	//close(connfd);
}


int main(int argc, char** argv)
{

	signal(SIGINT, closeHandler);

	//connection->sockfd = -1;
	//connection->path = getenv("PWD");
	start_webserver();

	connect_webserver();


	close(lfd);


	return 0;
}





