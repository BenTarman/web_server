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
#include <time.h>
#include <getopt.h>

#define KRED   "\x1b[31m"
#define KYEL   "\x1b[33m"
#define KGREEN "\x1b[32m"
#define KBLUE  "\x1b[34m"
#define KRESET "\x1b[0m"

#include <signal.h>

static volatile int keepRunning = 1;
	
int isVerbose = 0; //global variable for  verbose mode

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
	close(lfd);
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
	
	// assign random port number
	srand (time(NULL));
	int port = (rand() % (10000 - 1 + 1)) + 1000; 


	memset(&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = PF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);

	
	if (bind(lfd, (struct sockaddr *) &server, 
				sizeof(struct sockaddr_in)) == -1) 
	{
		debug(KRED "bind socket failed %s" KRESET "\n", strerror(errno));
	}

	printf(KGREEN "listening on port %d" KRESET "\n\n\n", port);
	
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


	printf("START OF HEADER\n");
	printf("================\n");
	memset( (void*)buffer, (int)'\0', 99999 );
	if ((read_size = recv(connfd, buffer, 99999, 0)) > 0)
	{
		debug(KGREEN "buffer size %d" KRESET "\n", strlen(buffer));
		printf("%s\n", buffer);
		char *req = strtok(buffer, "\n");
		char *req_type = malloc(6);
		char *file_to_serve = malloc(40);
		char *protocol_version = malloc(10);
		sscanf(req, "%s%s%s", req_type, file_to_serve, protocol_version);
		char* pos = strstr(file_to_serve, "."); //IP: the original string
		char fileName[40] = {0};
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
				debug(KRED "invalid http header" KRESET "\n");
				write(connfd, "HTTP/1.0 400 Bad Request\n", 25);
		}

		if (strcmp(req_type, "GET") == 0 && strcmp(fileName, "/favicon") != 0)
		{
				int count = 0;
				int fd = open(path, O_RDONLY);
				if (fd == -1)
				{
				write(connfd, "HTTP/1.0 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n",224);
					debug(KRED "file doesn't exist or bugged" KRESET "\n");
				}
				else
				{
					int count = 0;
					int fd_temp = open(path, O_RDONLY);
					while ((bytes_read = read(fd_temp, data, 512)) > 0) 
						count += bytes_read;
					close(fd_temp);
				
					debug(KBLUE "forming http response" KRESET "\n");
					// form http request	
					// =================
					char content_length[24];
					sprintf(content_length, "Content-Length: %d\n", count);

					//get file type
					char content_type[32];
					char *file_type = strrchr(file_to_serve, '.') + 1;
					size_t type_length = 0;

					if (strcmp(file_type, "gif") == 0)
					{
						debug(KBLUE "requesting gif" KRESET "\n");
						sprintf(content_type, "Content-Type: image/gif\n\n");
						type_length = 25;
					}
					else if (strcmp(file_type, "html") == 0)
					{
						debug(KBLUE "requesting html text" KRESET "\n");
						sprintf(content_type, "Content-Type: text/html\n\n");
						type_length = 25;
					}
					else if (strcmp(file_type, "jpg") == 0 )
					{
						debug(KBLUE "requesting jpg" KRESET "\n");
						sprintf(content_type, "Content-Type: image/jpeg\n\n");
						type_length = 26;
					}
					else if (strcmp(file_type, "png") == 0 )
					{
						debug(KBLUE "requesting png" KRESET "\n");
						sprintf(content_type, "Content-Type: image/png\n\n");
						type_length = 25;
					}
	
					debug(KBLUE "writing to webpage" KRESET "\n");
					// write http request here
					// =======================
					write(connfd, "HTTP/1.0 200 OK\n", 16);
					write(connfd, content_length, 34);
					write(connfd, content_type, type_length);

					while ((bytes_read = read(fd, data, 512)) > 0 )
					{
						debug(KYEL "writing %d bytes" KRESET "\n", bytes_read);
						write(connfd, data, bytes_read);
					}
					close(fd);
				}
		}
		else
		{
			// requests types aren't implemented so say its a bad request
			write(connfd, "HTTP/1.0 400 Bad Request\n", 25);
		}
	}

	printf("END OF HEADER\n");
	printf("================\n\n\n");
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

		debug( KGREEN "socket accepted" KRESET "\n");
		debug(KGREEN "Forking client" KRESET "\n\n");

		if (fork() == 0)
		{
			respond_to_client(connfd);
			exit(0);
		}
	}
}


int main(int argc, char** argv)
{
	int opt = 0;
	// check if verbose mode enabled
  while ((opt = getopt(argc,argv,"v")) != -1) {
    switch (opt) {
    case 'v':
			isVerbose = 1;
      break;
    case ':':
    case '?':
    default:
			debug("usage: %s -v\n", argv[0]);
      exit(-1);
    }
  }

	debug(KYEL "Setting up signal handlers" KRESET "\n\n\n");
	signal(SIGINT, closeHandler);

	debug(KYEL "Starting Web server" KRESET "\n");
	start_webserver();
	debug(KGREEN "Web server started" KRESET "\n\n");

	connect_webserver();

	return 0;
}

