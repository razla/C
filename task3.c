#include "line_parser.h"
#include <linux/limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void execute (cmd_line *line){
    
    int arr[2];
    pid_t pid1;
    pid_t pid2;

    // creating pipe
    if (pipe(arr) == -1) { 
        perror("failed");
        exit(1);
    }

    // first fork child1
    pid1 = fork(); 
    if (pid1 < 0) {
        perror("failed");
        exit(1);
    }

    // we're in child1
    if (pid1 == 0) { 
        if (line->next!='\0'){ // there's another command (pipe)
          fclose(stdout);
        }
        if (line->input_redirect!='\0'){ // input is changed
          fclose(stdin);
          fopen(line->input_redirect, "r"); 
        }
        if (line->output_redirect!='\0'){ // output is changed
          fclose(stdout);
          fopen(line->output_redirect, "w+");
        }
        
        dup(arr[1]); // duplicates write-end
        close(arr[1]); // closes the arr that was duplicated
        if (execvp(line->arguments[0], line->arguments) < 0) { // executes the command
            perror("failed");
            exit(1);
        }
    } 
    else { // Parent
        if (line!='\0' && line->blocking!=0){ // we need to wait for the child to finish
          waitpid(pid1, 0, 0);
        }
        if (line->next!='\0'){ // there's another command (pipe)
          close(arr[1]);
          // second forking for child2
          pid2 = fork();
          if (pid2 < 0) {
              perror("failed");
              exit(1);
          }

          if (pid2 == 0) { // Child 2
            if (line->next->input_redirect!='\0'){ // input is changed
              fclose(stdin);
              fopen(line->next->input_redirect, "r"); 
            }
            if (line->next->output_redirect!='\0'){ // output is changed
              fclose(stdout);
              fopen(line->next->output_redirect, "w+");
            }
              fclose(stdin);
              dup(arr[0]); // duplicates read-end
              close(arr[0]); // closes the arr that was duplicated

              if (execvp(line->next->arguments[0], line->next->arguments) < 0) { // executes the command
                  perror("failed");
                  exit(1);
              }

          } 
          else {
              close(arr[0]); // ?
              waitpid(pid1, 0, 0);
              waitpid(pid2, 0, 0);
          }
        }
      }
}

int main (int argc , char* argv[])
{

  char buffer[2048];
  char cwd[2048]; 
  cmd_line *line; 
  
  if (getcwd(cwd, sizeof(cwd)) != '\0'){
    fprintf(stdout, "> %s$:", cwd);
  }

  fgets(buffer, sizeof(buffer), stdin);

  while(strcmp(buffer,"quit\n") != 0){
    line = parse_cmd_lines(buffer);
    if (line!='\0'){
    	execute(line);
    	free_cmd_lines(line);
    }
    if (getcwd(cwd, sizeof(cwd)) != '\0'){
      fprintf(stdout, "> %s$:", cwd);
    }
    fgets(buffer, sizeof(buffer), stdin);
  }
  return 0;
}