#include "line_parser.h"
#include <linux/limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "job_control.h"

job** job_list;
struct termios *termios_p;
pid_t shell_pgid;

// creates a new pipe
int* createPipe () {
	int* pipes = malloc (2 * sizeof(int));
	pipe(pipes);
	return pipes; 
}

int execute (cmd_line* line, int* leftPipe, int* rightPipe, job* job_added, pid_t first_child){
    
    pid_t pid1;
    pid1 = fork(); 
    // we're in child1
    if (pid1 == 0) {
    	
		if (first_child==0){
			first_child=getpid();
		}
		setpgid(getpgid(getpid()), first_child);
    	job_added->pgid=first_child;
    	signal(SIGTTIN, SIG_DFL);
		signal(SIGTTOU, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGCHLD, SIG_DFL);	
    	if (rightPipe!='\0' && line->next!='\0'){ // if there's another command
    		close(rightPipe[0]);
    		fclose(stdout); // close stdout
    		dup(rightPipe[1]); // duplicate write-end of rightPipe
    		close(rightPipe[1]); // close the write-end of rightPipe
    	}
    	if (leftPipe!='\0'){ // if there's a command before - (if its the first command than its null)
    		close(leftPipe[1]);
    		fclose(stdin); // close stdin
    		dup(leftPipe[0]); // duplicate read-end of leftPipe
    		close(leftPipe[0]); // close the read-end of leftPipe
    	}
        if (line->input_redirect!='\0'){
              fclose(stdin);
              fopen(line->input_redirect, "r"); 
        }
		if (line->output_redirect!='\0'){
              fclose(stdout);
              fopen(line->output_redirect, "w+");
        }

        if (execvp(line->arguments[0], line->arguments) < 0) {
            perror("failed");
            exit(1);
        }

    } 

    else { // Parent
    	setpgid(getpgid(pid1), first_child);
    	job_added->pgid=first_child;
    	if (rightPipe!='\0') {
    		close(rightPipe[1]);
    	}
    	if (leftPipe!='\0') {
    		close(leftPipe[0]);
    		free(leftPipe);
    	}
        if (line->next!='\0'){
          leftPipe = createPipe();
          execute(line->next, rightPipe, leftPipe, job_added, first_child);
      	}
        if (line!='\0' && line->blocking!=0){
        	//job* job_fg = find_job_by_index(*job_list, atoi(line->arguments[1]));
		    run_job_in_foreground(job_list, job_added,	1 ,termios_p, shell_pgid);
          	waitpid(getpid(), 0, 0);
		}
	}
	return pid1;
}

void execute_init(cmd_line *line, job* job_added){
	pid_t pid_child=0;
	if (line->next!='\0'){
		int* pipe1 = createPipe();
		execute(line, NULL, pipe1, job_added, pid_child);
	}
	else {
		execute(line, NULL, NULL, job_added, pid_child);
	}
}

void signal_handler(int signum) {
   //printf("Ignore: %s\n", strsignal(signum));
}

int main (int argc , char* argv[])
{

  	char buffer[2048];
	char cwd[2048]; 
	cmd_line *line;

	signal(SIGSTOP, signal_handler);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGQUIT, signal_handler);
	signal(SIGCHLD, signal_handler);

	setpgid(0,0);

	termios_p = malloc(sizeof(struct termios));

	tcgetattr(STDIN_FILENO, termios_p);

	shell_pgid = getpgid(getpid());

	job_list=malloc(sizeof(job));
	*job_list=NULL;
	  
	if (getcwd(cwd, sizeof(cwd)) != '\0'){
	   fprintf(stdout, "> %s$:", cwd);
	}

	fgets(buffer, sizeof(buffer), stdin);
	job* added_job = add_job(job_list, buffer);
	added_job->status = 1;

	while(strcmp(buffer,"quit\n") != 0){

		line = parse_cmd_lines(buffer);
		if (strcmp(buffer, "jobs\n")==0){
			added_job->status = -1;
	    	print_jobs(job_list);	
	    }
	    else {
	    	if (strcmp(buffer,"\n")!=0){
	    		if (strcmp(line->arguments[0], "fg")==0){
		    		job* job_fg = find_job_by_index(*job_list, atoi(line->arguments[1]));
		    		//job_fg->status=-1;
		    		run_job_in_foreground(job_list, job_fg,	1 ,termios_p, shell_pgid);
		    		}
		    	else if (strcmp(line->arguments[0], "bg")==0){
		    			job* job_fg = find_job_by_index(*job_list, atoi(line->arguments[1]));
		    			//job_fg->status=-1;
		    			run_job_in_background(job_fg, 1);		
		    		}
		    		else{	
		    			execute_init(line, added_job);
		    		}
		    }
		    else {
		    	
		    	free_cmd_lines(line);
		    }
		}
		
	    if (getcwd(cwd, sizeof(cwd)) != '\0'){
	      fprintf(stdout, "> %s$:", cwd);
	    }

	    fgets(buffer, sizeof(buffer), stdin);
	    added_job = add_job(job_list, buffer);
		added_job->status = 1;
	}
	free_job_list(job_list);
	free(termios_p);
	return 0;
}