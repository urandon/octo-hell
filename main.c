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

struct pid_list{
	int pid;
	struct pid_list * next;
};

int free_pid_list(struct pid_list * list)
{
	int cnt = 0;
	struct pid_list * node = list;
	while(node != NULL){
		node = list->next;
		free(list);
		list = node;
		cnt++;
	}
	return cnt;
}

void delete(struct pid_list **list, int pid)
{
	struct pid_list * node = (*list), * prev = NULL;
	
	while(node != NULL){
		if(node->pid == pid){
			break;
		}
		prev = node;
		node = node->next;
	}
	
	if(node != NULL){
		if(prev == NULL){ /* => node == list => we need change "list" var */
			node = (*list)->next;
			free(*list);
			*list = node;
		} else {
			node = node->next;
			free(prev->next);
			prev->next = node;
		}
	}
}

/* returns when all the threads exited*/
struct pid_list * launch_chain_sync(struct exec_node * chain)
{
	struct exec_node *node = chain;
	char **args;
	int *pipefd_new, *pipefd_old, *t_swap;
	struct pid_list * list = NULL, * tmp;
	pid_t pid = -1;
	
	if(node == NULL){
		return list;
	}
	
	pipefd_new = (int *) malloc (2 * sizeof(int));
	pipefd_old = (int *) malloc (2 * sizeof(int));
	
	for(node = chain; node != NULL; node = node->next){
		pipe(pipefd_new);
		pid = fork();
		if(pid == 0){
			/* children */
			args = node->args;
			if(args == NULL){
				fprintf(stderr, "Cannot execute empty command\n");
				exit(1);
			}
			/* set up convayor IO streams */
			if(node != chain){ /* change input on non-first */
				dup2(pipefd_old[0], STDIN_FILENO);
				close(pipefd_old[0]);
				close(pipefd_old[1]);
			}
			if(node->next != NULL){ /* change output on non-last */
				dup2(pipefd_new[1], STDOUT_FILENO);
				close(pipefd_new[0]);
				close(pipefd_new[1]);
			}
			/* set up file IO redirrection */
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
		} else {
			/* controller */
			if(node != chain){
				close(pipefd_old[0]);
				close(pipefd_old[1]);
			}
			/* swap */
			t_swap = pipefd_old;
			pipefd_old = pipefd_new;
			pipefd_new = t_swap;
			/* pid_list */
			tmp = (struct pid_list *) malloc (sizeof(tmp));
			tmp->next = list;
			tmp->pid = pid;
			list = tmp;
		}
	}
	
	return list;
}

void change_dirrectory(const char * path){
	if(path == NULL){
		fprintf(stderr, "cd: path not specified (null got)\n");
	} else
	if(chdir(path) != 0){
		perror("cd");
	}
}

int get_n_execute(const char * home)
{
	struct exec_node * chain;
	struct pid_list * list;
	int status, bg_run;
	pid_t pid;

	chain = parse_string(&bg_run, &status);
	/* print_chain(chain); */

	if(say_if_error(status) == 0) {
		switch(look4cd(chain)){
			case 0:
				list = launch_chain_sync(chain);
				if(bg_run == 0){
					while(list != NULL){
						pid = wait(NULL);
						delete(&list, pid);
					}
				}
				free_pid_list(list);
				break;
			case 1:
				change_dirrectory(chain->args[1]);
				break;
			case 2:
				change_dirrectory(home);
				break;
			case 3:
				fprintf(stderr, "cd: too much arguments\n");
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
