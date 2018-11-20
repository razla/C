/*
** server.c -- a stream socket server demo
*/
#include "line_parser.h"
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
#include <sys/wait.h>
#include <signal.h>
#define _GNU_SOURCE
#include <dirent.h>     /* Defines DT_* constants */
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include "common.h"
#include <sys/sendfile.h>

#define PORT "2018"  // the port users will be connecting to
#define BUFF_SIZE 2048

#define BACKLOG 10     // how many pending connections queue will hold

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct linux_dirent {
    long           d_ino;
    off_t          d_off;
    unsigned short d_reclen;
    char           d_name[];
};

client_state* clientState;
int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
int counterId=1;
cmd_line *parsed_cmd;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int exec (char* cmd, char** args, int args_len) {
	if (strcmp(cmd, "hello")==0){
        if(clientState->conn_state!=IDLE){
        	perror("Client isn't idling");
        	return (-2);
        }
		clientState->conn_state=CONNECTED;
        clientState->client_id=counterId;
        printf("Client says: %s\n", cmd);
        printf(" --- executing connection... ---\n");
        char* message_to_send = malloc(BUFF_SIZE);
        memset(message_to_send, 0, strlen(message_to_send));
        strcat(message_to_send, "hello, ");
        char* client_id_str = malloc(5);
        sprintf(client_id_str, "%d", counterId);
        strcat(message_to_send, client_id_str);
        if (send(new_fd, message_to_send, strlen(message_to_send), 0) == -1)
            perror("send");
        counterId++;
        free(message_to_send);
        free (client_id_str);
        return 0;
    }
    if (strcmp(cmd, "bye")==0){
        if(clientState->conn_state!=CONNECTED){
        	perror("Client isn't connected");
        	return (-2);
        }
        clientState->conn_state=IDLE;
        printf("Client says: %s\n", cmd);
        printf(" --- executing disconnection... ---\n");
        char* message_to_send = "bye";
        if (send(new_fd, message_to_send, strlen(message_to_send), 0) == -1)
            perror("send");
        return 0;
    }
    if (strcmp(cmd, "ls")==0){
        if(clientState->conn_state!=CONNECTED){
        	perror("nok state");
        	return (-2);
        }
        printf("Client says: %s\n", cmd);
        char* message_to_send = "ok";
        if (send(new_fd, message_to_send, strlen(message_to_send), 0) == -1)
            perror("send");
        message_to_send = malloc(BUFF_SIZE);
        memset(message_to_send, 0, strlen(message_to_send));
        message_to_send[0]='\0';
        message_to_send=list_dir();
        if(message_to_send==NULL){
        	// NEED TO CHECK THAT NOK //
        }
        else {
	    	send(new_fd, message_to_send, strlen(message_to_send), 0);
	    }
        return 0;
    }
    if (strcmp(cmd, "get")==0){
        if(clientState->conn_state!=CONNECTED){
            perror("nok state");
            return (-2);
        }
        printf("Client says: %s\n", cmd);
        char* message_to_send = malloc(BUFF_SIZE);
        memset(message_to_send, 0, strlen(message_to_send));
        strcat(message_to_send, "ok ");
        int fileSize=file_size(args[1]);
        char size_to_send[50];
        sprintf(size_to_send, "%08d", fileSize);
        strcat(message_to_send, size_to_send);
        if (send(new_fd, message_to_send, strlen(message_to_send), 0) == -1)
            perror("send");

        // sent OK and file size //
        clientState->conn_state=DOWNLOADING;
        free(message_to_send);
        int file = open(args[1], O_RDWR);
        ssize_t sent = sendfile(new_fd, file, 0, fileSize);
        if (sent<0){
            perror("sent() failed");
            return(-2);
        }
        char* receive = malloc(BUFF_SIZE);
        memset(message_to_send, 0, BUFF_SIZE);
        int rc = recv(new_fd, receive, BUFF_SIZE, 0);
        if(rc < 0){
            perror("recv() failed");
            return(-2);
        }
        printf("Client says: %s\n", receive);
        if (strcmp(receive, "done")==0){
            char* message_to_send = malloc(2);
            memset(message_to_send, 0, strlen(message_to_send));
            strcat(message_to_send, "ok");
            send(new_fd, message_to_send, strlen(message_to_send), 0);
        }
        clientState->conn_state=CONNECTED;
        return 0;
    }
    if (strcmp(cmd, "ls")!=0 && strcmp(cmd, "bye") && strcmp(cmd, "hello")){
        // NEED TO WRITE NOK //
    }

    return 0;
}

int main(void)
{
    
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    clientState = malloc(sizeof(client_state));
    clientState->conn_state=IDLE;
    clientState->client_id=-1;
    clientState->sock_fd=-1;
    clientState->server_addr=malloc (BUFF_SIZE);
    gethostname(clientState->server_addr, BUFF_SIZE);
    socklen_t sin_size;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    int rv;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
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

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

        printf("server: got connection from %s\n", s);

		// keep running as long as the client keeps the connection open
		char* receive = malloc(BUFF_SIZE);
        memset(receive, 0, BUFF_SIZE);
        receive[0]='\0';
		int rc;
		while ((rc = recv(new_fd, receive, BUFF_SIZE, 0))>0) {
			parsed_cmd = parse_cmd_lines(receive);		
			exec(parsed_cmd->arguments[0], (char**)parsed_cmd->arguments, parsed_cmd->arg_count);
			free_cmd_lines(parsed_cmd);
			free(receive);
			receive = malloc(BUFF_SIZE);
            memset(receive, 0, BUFF_SIZE);
            receive[0]='\0';
		}
	}
    close(new_fd);
	return 0;
}


