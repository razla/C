#include "line_parser.h"
#include <linux/limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>

#define BUFF_SIZE 2048  // buffer size (2kb)
#define GET_BUFF_SIZE 1024 // get function buffer size (1kb)

typedef enum {
	IDLE,
	CONNECTING,
	CONNECTED,
	DOWNLOADING,
} c_state;
	
typedef struct {
	char* server_addr;	// Address of the server as given in the [connect] command. "nil" if not connected to any server
	c_state conn_state;	// Current state of the client. Initially set to IDLE
	int client_id;	// Client identification given by the server. NULL if not connected to a server.
	int sock_fd;		// The file descriptor used in send/recv
} client_state;



/// creates the socket, first arg: internet domain, ///
/// second arg: stream socket, third arg: default protocol ///
int clientSocket;
int debug_mode=0;
char* ls;
client_state* clientState;
char buffer[BUFF_SIZE];
int status;
struct addrinfo hints;
struct addrinfo *servinfo;  // will point to the results
cmd_line *parsed_cmd;

void print_to_stderr(char* to_print){
	if (debug_mode==1){
		fprintf(stderr, "%s|Log: %s\n", clientState->server_addr, to_print);
	}
}

int exec (char* cmd, char** args, int args_len) {
	if(strcmp(cmd, "conn")==0){	
		if (clientState->conn_state!=IDLE){
			perror("Client is not idling");
			return(-2);
		}
		memset(&hints, 0, sizeof(hints));// make sure the struct is empty
		hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
		hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
		hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
		if ((status = getaddrinfo(args[1], "2018", &hints, &servinfo)) != 0) { // gets the server info to servinfo
	    	fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    		return(-1);
		}
		/*---- Creates a socket ----*/
		clientSocket = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
      	if (clientSocket < 0)
      	{
        	perror("socket() failed");
        	return(-2);
      	}
		/*---- Connects to the server ----*/
		if(connect(clientSocket, servinfo->ai_addr, servinfo->ai_addrlen)==-1){
			perror("couldn't connect to server");
			return(-1);
		}
		char* data_to_send = "hello";
		int sent = send(clientSocket, data_to_send, strlen(data_to_send), 0);
      	if (sent < 0)
      	{
        	perror("send() failed");
         	return(-2);
      	}
      	char* receive = malloc(BUFF_SIZE);
      	memset(receive, 0, strlen(receive));
      	/*---- Read the message from the server into the buffer ----*/
      	int rc = recv(clientSocket, receive, BUFF_SIZE, 0);
        if (rc < 0)
        {
            perror("recv() failed");
            return(-2);
        }
		  /*---- Print the received message ----*/
  		
  		cmd_line* connection = parse_cmd_lines(receive); 
		
  		if (strcmp(connection->arguments[0], "hello,")==0){
  			char* serverAddress = malloc(sizeof(args[1]));
  			strcpy(serverAddress, args[1]);	
  			clientState->server_addr=serverAddress;
			clientState->conn_state=CONNECTED;
			clientState->client_id=-1; /*---- NEED TO CHECK ----*/
			clientState->sock_fd=clientSocket;
			print_to_stderr(receive);
			memset(receive, 0, strlen(receive));
			free(receive);
  			free_cmd_lines(connection);
  		}
  		return 0;
	}
	if (strcmp(cmd, "bye")==0){
		if (clientState->conn_state!=CONNECTED){
			perror("Client is not connected");
			return(-2);
		}
		/*---- Client wants to disconnect from the server ----*/
		const char* data_to_send = "bye";
		send(clientSocket, data_to_send, strlen(data_to_send), 0);
		clientState->server_addr="nil";
		clientState->conn_state=IDLE;
		clientState->client_id=-1;
		clientState->sock_fd=-1;
		debug_mode=0;
		close(clientSocket);
		return 0;
	}
	if (strcmp(cmd, "-d")==0){
		if (debug_mode==0){
			debug_mode=1;
			return 0;	
		}
		debug_mode=0;
		return 0;
	}
	if (strcmp(cmd, "ls")==0){
		if (clientState->conn_state!=CONNECTED){
			perror("Client is not connected");
			return(-2);
		}
		/*---- Client wants to disconnect from the server ----*/
		const char* data_to_send = "ls";
		send(clientSocket, data_to_send, strlen(data_to_send), 0);
		char* receive = malloc(2);
		memset(receive, 0, strlen(receive));
      	/*---- Read the message from the server into the buffer ----*/
      	int rc = recv(clientSocket, receive, 2, 0);
        if (rc < 0)
        {
            perror("recv() failed");
            return(-2);
        }
  		print_to_stderr(receive);
  		ls = malloc(BUFF_SIZE);
  		memset(ls, 0, strlen(ls));
  		ls[0]='\0';
  		if (strcmp(receive, "ok")==0){
  			
  			//receive = malloc(BUFF_SIZE);
  			int rc = recv(clientSocket, ls, BUFF_SIZE, 0);
        	if (rc < 0)
        	{
            	perror("recv() failed");
            	return(-2);
        	}
        	print_to_stderr(ls);
        		
  		}
  		free(receive);
  		free (ls);
		return 0;
	}
		if (strcmp(cmd, "get")==0){
		if (clientState->conn_state!=CONNECTED){
			perror("Client is not connected");
			return(-2);
		}
		/*---- Client wants to disconnect from the server ----*/
		char* data_to_send = malloc(BUFF_SIZE);
		memset(data_to_send, 0, strlen(data_to_send));
		strcat(data_to_send, "get ");
		strcat(data_to_send, args[1]);
		send(clientSocket, data_to_send, strlen(data_to_send), 0);
		free(data_to_send);

		// creates the new file name
		char* file_name_temp = malloc(BUFF_SIZE);
		memset(file_name_temp, 0, strlen(file_name_temp));
		strcat(file_name_temp, args[1]);
		char* file_name_normal = malloc(BUFF_SIZE);
		memset(file_name_normal, 0, strlen(file_name_normal));
		strcat(file_name_normal, args[1]);
		strcat(file_name_temp, ".tmp");
		// got the name of the file
		char* receive = malloc(12);
		memset(receive, 0, strlen(receive));
		receive[0]='\0';
      	/*---- Read the message from the server into the buffer ----*/
      	int rc = recv(clientSocket, receive, 11, 0);
        if (rc < 0)
        {
            perror("recv() failed");
            return(-2);
        }
        receive[11]='\0';
  		print_to_stderr(receive);
  		free_cmd_lines(parsed_cmd);
  		parsed_cmd = parse_cmd_lines(receive);		
  		if (strcmp(parsed_cmd->arguments[0], "ok")==0){
  			clientState->conn_state=DOWNLOADING;
  			char* size_in_string;
  			int fileSize = strtol(parsed_cmd->arguments[1], &size_in_string, 10);
  			FILE* file_descriptor = fopen(file_name_temp, "ab+");
  			if(file_descriptor==NULL){
  				perror("fopen() failed");
  			}
  			// now we need to get the file itself //
  			int new_file_size=fileSize;
  			int bytesRead=0;
  			while(bytesRead<fileSize){
  				char* file_buffer = malloc(GET_BUFF_SIZE);
  				memset(file_buffer, 0, strlen(file_buffer));
  				file_buffer[0]='\0';
  				rc = recv(clientSocket, file_buffer, GET_BUFF_SIZE, 0);
  				if (rc < 0) {
  					perror("recv() failed");
  					return(-2);
  				}
  				if (new_file_size<GET_BUFF_SIZE){
  					bytesRead += fwrite(file_buffer, sizeof(char), new_file_size, file_descriptor);
  				}
  				else {
  					bytesRead += fwrite(file_buffer, sizeof(char), GET_BUFF_SIZE, file_descriptor);
  				}
  				if (bytesRead<0){
  					perror("write() failed");
  					return(-2);
  				}
  				new_file_size=new_file_size-GET_BUFF_SIZE;
  				free (file_buffer);
  			}
  			fclose(file_descriptor);
			send(clientSocket, "done", strlen("done"), 0);
  			free(receive);
			receive = malloc(BUFF_SIZE);
			memset(receive, 0, BUFF_SIZE);
			int rc = recv(clientSocket, receive, BUFF_SIZE, 0);
			if (rc < 0) { 
				perror("recv() failed");
				return (-2);
			}
			print_to_stderr(receive);
			if (strcmp(receive, "ok")==0){
				if(rename(file_name_temp, file_name_normal)<0){
					perror("rename() failed");
					return (-2);
				}
				clientState->conn_state=CONNECTED;
				return 0;
			}

  		}
		return 0;
	}
	return 0;
}

int main (int argc , char* argv[])
{
	clientState = malloc(sizeof(client_state));
	/// initializes the client's states ///
	clientState->server_addr="nil";
	clientState->conn_state=IDLE;
	clientState->client_id=-1;
	clientState->sock_fd=-1;

	
	printf("server:%s>", clientState->server_addr);
	fgets(buffer, sizeof(buffer), stdin);

	while(strcmp(buffer,"quit\n") != 0){
		parsed_cmd = parse_cmd_lines(buffer);		
		exec(parsed_cmd->arguments[0], (char**)parsed_cmd->arguments, parsed_cmd->arg_count);
		free_cmd_lines(parsed_cmd);
	    printf("server:%s>", clientState->server_addr);
	    memset(buffer, 0, sizeof(buffer));
	    fgets(buffer, sizeof(buffer), stdin);

	}

	freeaddrinfo(servinfo); // free the linked-list
	return 0;
}