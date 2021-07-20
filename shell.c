// Tales Cabral Nogueira
// Isabella Crosariol

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

//////////////////////////////////////////////////////////////////////////////////////////////// Executor de Comandos

int exec(int start, int end, char **argv){

	char **cmd;
	
	cmd = &argv[start];
	argv[end] = NULL;

	pid_t child = fork();

	if (child < 0) { 
		perror("fork()");
		return -1;
	}else if(child == 0){
		if(execvp(cmd[0], cmd) != 0){
			perror("execvp()");
			exit(-1);
		}
	}
	
	int status;
	waitpid(child, &status, 0);
	return status;
}

//////////////////////////////////////////////////////////////////////////////////////////////// Executa Comando em Background

void execBACK(int start, int end, char **argv){

	char **cmd;
	
	cmd = &argv[start];
	argv[end] = NULL;
	
	pid_t child = fork();
	
	if (child < 0) { 
		perror("fork()");
		exit(-1);
	}else if(child == 0){
		if(execvp(cmd[0], cmd) != 0){
			perror("execvp()");
			exit(-1);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////// Executa Comando em Pipe

void execPIPE(char **argv, int pipeCounter, int *operArray){

	int fd[pipeCounter][2];
	
	int comandos = pipeCounter + 1;	

	pid_t child;
	
	int status;
	
	int i;
	
	for(i = 0; i < comandos; i++){
	
		if(i < pipeCounter){
			if(pipe(fd[i]) < 0){
				perror("pipe()");
				exit(-1);
			}
		}
		
		child = fork();
		
		if(child < 0){
			perror("fork()");
			exit(-1);
		}else if(child == 0){

			if(i < comandos-1){
				dup2(fd[i][1], STDOUT_FILENO);
				close(fd[i][0]);
				close(fd[i][1]);
			}

			if(i > 0){
				dup2(fd[i-1][0], STDIN_FILENO);
				close(fd[i-1][1]);
				close(fd[i-1][0]);
			}

			char **cmd;
			cmd = &argv[operArray[i]];

			if(execvp(cmd[0], cmd) != 0){
				perror("execvp()");
				exit(-1);
			}
		}else{
		
			if(i > 0){

				close(fd[i-1][0]);
				close(fd[i-1][1]);
			}
			waitpid(-1, &status, 0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////// Podador de Pipeline

int pipeLINE(int start, int firstPipe, int end, char **argv){
	
	int pipeCounter = 1;
	int operArray[end];
	
	argv[firstPipe] = NULL;
	
	operArray[0] = start;
	operArray[1] = firstPipe + 1;
	
	int i;
	
	for(i = firstPipe + 1; i < end; i++){
		if(strcmp(argv[i], "|") == 0){
			argv[i] = NULL;
			pipeCounter++;
			operArray[pipeCounter] = i + 1;
		}else if(strcmp(argv[i], ";") == 0){
			argv[i] = NULL;
			break;
		}else if( strcmp(argv[i], "||") == 0 || strcmp(argv[i], "&&") == 0 || strcmp(argv[i], "&") == 0) break;
	}
	
	execPIPE(argv, pipeCounter, operArray);

	return i;
}

//////////////////////////////////////////////////////////////////////////////////////////////// Shell Main

int main(int argc, char **argv) {

	int i;
	int status = 0;
	int argument = 1;

	if (argc == 1) {
		printf("Use: %s <command> <arg_1> <arg_2> ... <arg_n>\n", argv[0]);
		return 0;
	}else{
		for(i = 1; i < argc; i++){
            if(strcmp(argv[i], "&&") == 0){
				if(status == 0){
					status = exec(argument, i, argv);
				}
				argument = i + 1;
			
			}else if(strcmp(argv[i], "||") == 0){
				if(status == 0){
					status = exec(argument, i, argv);
				}
				argument = i + 1;

				if(status != 0) status = 0;
				else status = 1;
			
			}else if(strcmp(argv[i], ";") == 0){
				if(status == 0){
					status = exec(argument, i, argv);
				}
				argument = i + 1;
				status = 0;
			
			}else if(strcmp(argv[i], "&") == 0){
				if(status == 0){
					execBACK(argument, i, argv);
				}
				argument = i + 1;
				
			}else if(strcmp(argv[i], "|") == 0){
				if(status == 0){
					i = pipeLINE(argument, i, argc, argv);
				}
				argument = i + 1;
			}
		}
	}
	
	if(status == 0 && i != argc + 1){
		exec(argument, i, argv);
	}
	
	printf("---END---\n");
	return 0;
}