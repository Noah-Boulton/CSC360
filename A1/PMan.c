#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
//#include <readline/history.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include "linkedList.h"

/*Function Prototypes*/
void check();
int getInput(char** args);
int run(char** args);
int bg(char** opt);
int bglist();
int bgkill(int pid);
int bgstop(int pid);
int bgstart(int pid);
int pstat(int pid);
int printProgram(int pid);

/*Linked List variables*/
node* head;
node* temp;

int main(){
	//char* args[128];
	while(1){
		char* args[128];
		getInput(args);
		run(args);
		check();
	}
	return 1;
}

/*Get input from the user and tokenize into an array*/
int getInput(char** args){
	char *input = NULL ;
        char *prompt = "PMan: >";
	input = readline(prompt);
        if(strcmp(input, "") == 0){
                args[0] = "";
		check();
                return 1;
        }
        char* opt = strtok(input, " ");
        int i;
        for(i = 0; i < 128; i++){
                args[i] = opt;
                opt = strtok(NULL, " ");
        }
	return 1;
}

/*Take the user input array and call the cooresponding function*/
int run(char** args){
	if(strcmp(args[0], "") == 0){
		return 1;
	}
	int pid;
        if(strcmp(args[0],"bg") == 0){
       		if(args[1] == NULL){
                        printf("Please input a program to run\n");
                } else {
                        bg(args);
       	        }
        } else if(strcmp(args[0],"bglist") == 0){
                bglist();
        } else if(strcmp(args[0],"bgkill") == 0){
                if(args[1] == NULL){
                 	printf("Error: Please enter a pid.\n");
                } else {
                       	pid = atoi(args[1]);
               	        bgkill(pid);
                }
        } else if(strcmp(args[0],"bgstop") == 0){
                if(args[1] == NULL){
                        printf("Error: Please enter a pid.\n");
                } else {
                        pid = atoi(args[1]);
                        bgstop(pid);
                }
        } else if(strcmp(args[0],"bgstart") == 0){
                if(args[1] == NULL){
                        printf("Error: Please enter a pid.\n");
                } else {
                       pid = atoi(args[1]);
                       bgstart(pid);
                }
        } else if(strcmp(args[0],"pstat") == 0){
                if(args[1] == NULL){
                	printf("Error: Please enter a pid.\n");
                } else{
                        pid = atoi(args[1]);
                       	pstat(pid);
                }
        } else {
                printf("%s: command not found\n", args[0]);
        }
	return 1;
}

/*Called in each iteration of main to check the status of child processes,
  if a zombie process is found it it removed from the linked list.*/
void check(){
	int status;
	pid_t pid = waitpid(-1, &status, WNOHANG);
	/*errno = ECHILD means no child process*/
	if(pid < 0 && errno != ECHILD){
		printf("Waitpid failed\n");
	} else if(pid > 0){
		printf("Process %d done.\n", pid);
		temp = search(head,pid);
        	if(temp != NULL) {
      	      		head = remove_any(head,temp);
        	}
	}
}

/*Uses execvp to run the program entered by the user*/
int bg(char** opt){
	pid_t pid;
	int status;
	pid = fork();
        if(pid < 0){
                fprintf(stderr, "Fork Fail\n");
        } else if(pid == 0){
                /*Child process*/
		char prog[100] = "";
		strcat(prog, opt[1]);
		execvp(prog, &opt[1]);
		printf("Program failed.\n");
		exit(0);
	} else {
		head = prepend(head,pid);
		printf("Program %s started in background with pid %d.\n", opt[1], pid);
                return 1;
        }
        return 1;
}

/*Lists all current processes with comm and pid*/
int bglist(){
	int cnt = count(head);
	node* cursor = head;
    	while(cursor != NULL){
        	printf("%d: ", cursor->data);
		printProgram(cursor->data);
        	cursor = cursor->next;
    	}
	printf("Total background jobs: %d\n", cnt);
	return 1;
}

/*Kills process with specified pid if the process is in the linked list*/
int bgkill(int pid){
	temp = search(head,pid);
        if(temp != NULL) {
            head = remove_any(head,temp);
        }
        else
        {
        	printf("Error: Process %d does not exist.\n",pid);
	}
	kill(pid, SIGTERM);
	printf("Process %d killed.\n", pid);
	usleep(1000);
	check();
	return 1;
}

/*Stops process with specified pid if the process is in the linked list*/
int bgstop(int pid){
	temp = search(head,pid);
        if(temp == NULL){
                printf("Error: Process %d does not exist.\n",pid);
		return 0;
        }
	kill(pid, SIGSTOP);
	printf("Process %d stopped.\n", pid);
	return 1;
}

/*Starts process with specified pid if the process is in the linked list*/
int bgstart(int pid){
	temp = search(head,pid);
        if(temp == NULL){
                printf("Error: Process %d does not exist.\n",pid);
		return 0;
        }
	kill(pid, SIGCONT);
	printf("Process %d started.\n", pid);
	return 1;
}

/*Prints out comm, state, utime, stime, rss, voluntart_ctxt_switches, and nonvoluntary_ctxt_switches for the
  process with specified pid*/
int pstat(int pid){
	temp = search(head,pid);
	if(temp == NULL){
		printf("Error: Process %d does not exist.\n",pid);
		return 0;
	}
	char file[100];
	sprintf(file, "/proc/%d/stat", pid);
	FILE *f = fopen(file, "r");
	if(f == NULL){
                printf("Error reading from file.\n");
                return 0;
        }
	int tmp;
	char c[200];
	char s;
	float utime;
	float stime;
	int rss;
	fscanf(f, "%d %s %c %d %d %d %d %d %d %d %d %d %d %f %f %d %d %d %d %d %d %d %d %d ", &tmp, c, &s, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &utime, &stime, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &tmp, &rss);
	utime = utime/100;
	stime = stime/100;
	fclose(f);

	sprintf(file, "/proc/%d/status", pid);
	FILE *f1 = fopen(file, "r");
	if(f1 == NULL){
		printf("Error reading from file.\n");
		return 0;
	}

	char li[200];
	char comm[200];
        char state[200];
	char vol[200];
	char non[200];
	int i;
	for(i = 0; i < 41; i++){
		fgets(li, 200, f1);
		if(i == 0){
			strcpy(comm, li);
		}
		if(i == 1){
			strcpy(state, li);
		}
		if(i == 39){
			strcpy(vol, li);
		}
		if(i == 40){
			strcpy(non, li);
		}
	}
	printf("%s%sutime:	%f\nstime:	%f\nrss:	%d\n%s%s", comm, state, utime, stime, rss, vol, non);
	return 1;

}

/*Used by bglist to access comm of the process*/
int printProgram(int pid){
	char file[100];
        sprintf(file, "/proc/%d/stat", pid);
        FILE *f = fopen(file, "r");
	char comm[100];
	int tmp;
	fscanf(f, "%d %s", &tmp, comm);
	printf("%s.\n", comm);
	return 1;
}
