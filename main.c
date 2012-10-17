#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "parser.h"

#define DBG ;fprintf(stderr,"google.com");

int cmpstr(const char *a, const char *b)
{
	while((*a == *b) && (*a != '\0')){
		++a;
		++b;
	}
	return *a == *b;
}



void print_chain(struct exec_node *chain){
	struct exec_node *p;
	printf("Chain@:\n");
	for(p = chain; p != NULL; p = p->next){
		char **str;
		printf("Node:");
		for(str = p->args; *str != NULL; str++){
			printf(" %s", *str);
		}
		printf("\ninput:%s\n", p->input);
		printf("output:%s\n", p->output);
		printf("next~>\n");
	}
	printf("~end\n");
}


/* 0 - no cd
 * 1 - correct cd (one argument and no convayer)
 * 2 - there is cd, but not enough #args (zero)
 * 3 - there is cd, but #args more than one
 * 4 - there is cd, but convayer found
 *  */
int look4cd(struct exec_node *chain)
{
	struct exec_node *node;
	char ** args;
	int cd_here = 0, cd_args = 0;
	
	if(chain == NULL){
		return 0;
	}
	
	for(node = chain; node != NULL && !cd_here; node = node->next){
		if(node->args != NULL){
			cd_here = cmpstr(node->args[0], "cd");
			if(cd_here == 1){
				args = node->args;
				if(args[1] == NULL){
					cd_args = 0;
				} else
				if(args[2] == NULL) {
					cd_args = 1;
				} else {
					cd_args = 2;
				}
			}
		}
	}
	
	if(cd_here){
		if(node != chain->next || node != NULL){
			/* convayor found */
			return 4;
		}
		switch(cd_args){
			case 0: return 2; break;
			case 1: return 1; break;
			case 2: return 3; break;
		}
	}
	
	return 0;   
}

int say_if_error(int status)
{
	if(status & BAD_QUOTES)
		printf("Missed quotes\n");
	if(status & BAD_AMPERSAND)
		printf("Wrong ampersand was found\n");
	if(status & BAD_CONVEYOR)
		printf("The conveyor cannot be created: not enough commands to execute\n");
	if(status & DUPLICATE_STDIN)
		printf("Duplicate standart input\n");
	if(status & DUPLICATE_STDOUT)
		printf("Duplicate standart output\n");
	
	return status;
}

void launch_chain(struct exec_node * chain, int bg_run) /* runned in forked process */
{
	struct exec_node *node = chain;
	char **args;
	int pipefd[2];
	pid_t pid = -1;

	while(node != NULL){
		if(node->args == NULL){
			fprintf(stderr, "Cannot execute empty command\n");
			exit(1);
		}
		if(node->next != NULL){
			pipe(pipefd);
			pid = fork();
			if(pid == -1){
				perror("fork");
				exit(1);
			}
			if(pid == 0){
				/* children */
				dup2(pipefd[0], STDIN_FILENO);
				close(pipefd[0]);
				close(pipefd[1]);
				node = node->next;
			} else {
				/* parent */
				dup2(pipefd[1], STDOUT_FILENO);
				close(pipefd[0]);
				close(pipefd[1]);
			}
		}
		if(pid != 0 || node->next == NULL){
			/* parent or the last */
			/* check for IO redirrection */
			args = node->args;
			/* input */
			if(node->input != NULL){
				if(node == chain){
					int newfd = open(node->input, O_RDONLY);
					if(newfd == -1 || (newfd != -1 && (dup2(newfd, STDIN_FILENO) == -1))){
						fprintf(stderr, "%s: bad input stream: %s\n", args[0], strerror(errno));
						exit(1);
					}
					close(newfd);
				} else {
					fprintf(stderr, "%s: input redirrection ingorring, convayor found\n", args[0]);
				}
			}
			/* output */
			if(node->output != NULL){
				if(node->next == NULL){
					int newfd = open(node->output, O_CREAT|O_WRONLY|O_TRUNC, 0666);
					if(newfd == -1 || (newfd != -1 && (dup2(newfd, STDOUT_FILENO) == -1))){
						fprintf(stderr, "%s: bad output stream: %s\n", args[0], strerror(errno));
						exit(1);
					}
					close(newfd);
				} else {
					fprintf(stderr, "%s: output redirrection ingorring, convayor found\n", args[0]);
				}
			}
			/* execution */
			execvp(args[0], args);
			perror(args[0]);
			exit(1);
		}
	}		
	
	exit(1);
}

void change_dirrectory(const char * path){
	if(path == NULL){
		fprintf(stderr, "cd: path not specefied (null got)\n");
	} else
	if(chdir(path) != 0){
		perror("cd");
	}
}

int get_n_execute(const char * home)
{
	struct exec_node * chain;
	int status, bg_run;
	pid_t pid;

	chain = parse_string(&bg_run, &status);
	/* print_chain(chain); */

	if(say_if_error(status) == 0) {
		switch(look4cd(chain)){
			case 0:
				if((pid = fork()) == 0){
					launch_chain(chain, bg_run);
				} else
				if(bg_run == 0){					
					waitpid(pid, NULL, 0);
				}
				break;
			case 1:
				change_dirrectory(chain->args[1]);
				break;
			case 2:
				change_dirrectory(home);
				break;
			case 3:
				fprintf(stderr, "cd: Too much arguments\n");
				break;
			case 4:
				fprintf(stderr, "cd: Cannot be user in convayor\n");
				break;
			default:
				break;
		}
	}
	/* free memory */
	destruct_chain(chain);
	
	return status;
}

int main(int argc, const char * const * argv){
	int status = 0;
	int zombie;
	char *home;
	
	home = getenv("HOME");

	const char *hello = "root@8.8.8.8:~# ";

	while(! (status & EOF_ERROR)){
		printf("%s", hello);
		status = get_n_execute(home);

		while((zombie = waitpid(-1, NULL, WNOHANG)) != -1 && zombie != 0){
			printf("[PID %d] finished\n", zombie);
		}
	}
	
	printf("\n[logout]\n");

	return 0;
}
