/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
// waitpid() might overwrite errno, so we save and restore it:
#include <sys/wait.h>
#include <signal.h>
#include<pthread.h>

#define PORT "3590"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler(int s)
{
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int Mysockinit(int sockfd,struct addrinfo hints, struct addrinfo *servinfo,struct addrinfo *p)
{
	int rv;
	int yes=1;

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure
	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	return sockfd;
}

void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char *message , client_message[2000];

    //Send some messages to the client
    message = "Greetings! I am your connection handler\n";
    write(sock , message , strlen(message));

    message = "Now type something and i shall repeat what you type \n";
    write(sock , message , strlen(message));

    //Receive a message from client
    while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
    {
        //Send the message back to client
				printf("received: %s\n",client_message);
				write(sock , client_message , strlen(client_message)+1);
				//memset(client_message,' ',sizeof(client_message));
		}

    if(read_size == 0)
    {
        //printf("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
		//close(sock);
    //Free the socket pointer
    free(socket_desc);

    return 0;
}

int main(void)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	{
	hints.ai_flags = AI_PASSIVE; // use my IP

	sockfd=Mysockinit(sockfd,hints,servinfo,p);
	if(sockfd==-1)
		return 1;
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	int *new_sock;
	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		while((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)))
		{
			pthread_t sniffer_thread;
			new_sock = malloc(1);
			*new_sock = new_fd;
			if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
			 {
					 perror("could not create thread");
					 return 1;
			 }
			 printf("thread create after");
			 	pthread_join( sniffer_thread , NULL);
        printf("Handler assigned");
		}
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		close(new_fd);  // parent doesn't need this
	}

	return 0;
}
