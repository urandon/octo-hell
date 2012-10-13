#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#define bool char
#define true 1
#define false 0

#define DBG ;fprintf(stderr,"google.com");

enum error_status {
    EOF_ERROR       = 1,
    BAD_QUOTES      = 2,
    BAD_AMPERSAND   = 4,
    BAD_CONVEYOR    = 8,
    DUPLICATE_STDIN = 16,
    DUPLICATE_STDOUT= 32,
    UNKNOWN_ERROR   = 64,
};


struct exec_node{
    char **args; /* execution arguments */
    char *input, *output;
    struct exec_node *next;
};

struct words{
    char *data;
    struct words *next;
};

int cmpstr(const char *a, const char *b)
{
    while((*a == *b) && (*a != '\0')){
        ++a;
        ++b;
    }
    return *a == *b;
}

void free_array(char** args)
{
    char** str;
    for(str = args; *str != NULL; str++){
        free(*str);
    }
    free(args);
}


char** convert_words2array(struct words * list, long n)
{
    char ** a = (char **) malloc (sizeof(char *) * (n+1));
    struct words *p;
    long int i;
    
    for(i = 0, p = list; i < n; i++, p = p->next){
        a[i] = p->data;
    }
    a[n] = NULL;
    
    return a;
}



void destruct_words(struct words *p)
{
    struct words *q;
    while(p != NULL){
        q = p->next;
        free(p);
        p = q;
    }
}

void destruct_chain(struct exec_node *p)
{
    struct exec_node *q;
    while(p != NULL){
        q = p->next;
        free_array(p->args);
        free(p->input);
        free(p->output);
        free(p);
        p = q;
    }
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


struct exec_node * parse_string(int * bg_run, int * err_status)
{
    struct exec_node *chain, *node, *prev_node = NULL;
    char** args;
    char *string;
    struct words *list, *word, *prev_word = NULL;
    /*buffer defenitions */
    const long int DEFAULT_BUFFER_SIZE = 64;
    char *buffer = (char*) malloc (sizeof(char) * DEFAULT_BUFFER_SIZE);
    long int buffer_size = DEFAULT_BUFFER_SIZE, buffer_actual_size = 0;
    int quotes_on = false;
    enum dirrection{
        ARGUMENTS,
        STDIN,
        STDOUT
    } dirr = ARGUMENTS;
    long int count;
    int ch;
    
    *err_status = 0;
    *bg_run = false;
    
    node = (struct exec_node*) malloc (sizeof(*node));
    node->input = NULL;
    node->output = NULL;
    node->args = NULL;
    node->next = NULL;
    chain = node;
        
    word = (struct words*) malloc (sizeof(*word)); /* head word */
    word->data = NULL;
    word->next = NULL;
    list = word;
    count = 0;

    do{
        ch = getchar();
        if(ch == EOF){
            *err_status |= EOF_ERROR;
        }
        if(*bg_run == true && ch != EOF && !isspace(ch)){
            *err_status |= BAD_AMPERSAND;
        }
        if(ch == '&'){
            *bg_run = true;
        }

        if(ch == '"'){
            quotes_on ^= true;
        } else
        if((!quotes_on && (isspace(ch) || ch == '&' || ch == '|' || ch == '<' || ch == '>'))
          || ch == EOF || ch == '\n'){
            /* drop buffer to structure if not empty */
            if(buffer_actual_size != 0){
                long int i;
                string = (char*) malloc((buffer_actual_size + 1) * sizeof(char));
                for(i = 0; i < buffer_actual_size; i++){
                    string[i] = buffer[i];
                }
                string[buffer_actual_size] = '\0';
                
                switch(dirr){
                    case ARGUMENTS:
                        word->next = (struct words *) malloc (sizeof(*word));
                        word->data = string;
                        prev_word = word;
                        
                        word = word->next;
                        word->next = NULL;
                        count++;
                        break;
                    case STDIN:
                        if(node->input != NULL){
                            *err_status |= DUPLICATE_STDIN;
                            free(node->input);
                        }
                        node->input = string;
                        break;
                    case STDOUT:
                        if(node->output != NULL){
                            *err_status |= DUPLICATE_STDOUT;
                            free(node->output);
                        }
                        node->output = string;
                        break;
                }
                
                dirr = ARGUMENTS;
                buffer_actual_size = 0;
            }
        } else {
            /* add symbol to buffer */
            if(buffer_actual_size >= buffer_size){
                /* realloc buffer */
                char *p;
                long int i;

                p = (char*) malloc (sizeof(char) * (buffer_size << 2));
                /* copy buffer to a new place */
                for(i = 0; i < buffer_size; i++){
                    p[i] = buffer[i];
                }
                free(buffer);
                buffer = p;
                buffer_size <<= 2;
            }
            buffer[buffer_actual_size++] = ch;
        }
        
        /* specian situations */
        if(!quotes_on){
            switch(ch){
                case '&':
                    *bg_run = true;
                    break;
                case '>':
                    dirr = STDOUT;
                    break;
                case '<':
                    dirr = STDIN;
                    break;
                default:
                    break;
            }
        }
        
        /* new program args token */
        if((ch == '|' && !quotes_on) || ch == '\n' || ch == EOF){
            /* we need to create new convayor chain element */
            /* check for empty string */
            free(word);
            if(prev_word == NULL){
                list = NULL;
            } else {
                prev_word->next = NULL;
            }
            args = convert_words2array(list, count);
            destruct_words(list);

            /* push 'args' into chain */
            node->args = args;
            
            if(ch == '|'){
                prev_node = node;
                node->next = (struct exec_node*) malloc(sizeof(*node));             
                node = node->next;
                node->next = NULL;
                node->input = NULL;
                node->output = NULL;
            }

            word = (struct words*) malloc (sizeof(*word)); /* head word */
            word->data = NULL;
            word->next = NULL;
            list = word;
            count = 0;
            dirr = ARGUMENTS;
        }
    } while(ch != '\n' && ch != EOF);

    /* free memory */
    free(buffer);
    /* check if the the string was empty */
    if(chain->args[0] == NULL && chain->next == NULL){
        destruct_chain(chain);
        chain = NULL;
    }
    /* check for errors */
    /* convayor error */
    if(chain != NULL){
        for(node = chain; node != NULL; node = node->next){
            *err_status |= (node->args[0] == NULL) ? BAD_CONVEYOR : 0;
        }
    }
    
    /* quote error */
    *err_status |= (quotes_on) ? BAD_QUOTES : 0;
    
    args = convert_words2array(list, count);
    destruct_words(list);
    
    return chain;
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
    
    return status & ~EOF_ERROR;
}

void launch_chain(struct exec_node * chain, int bg_run)
{
    struct exec_node *node;
    char **args;

    for(node = chain; node != NULL; node = node->next){
        args = node->args;
        if(*args != NULL) {
            /* everything is OK */
            /* simple version */
            int pid = fork();
            if(pid == 0){
                /* redirrect IO if needed */
                if(node->input != NULL){
                    fclose(stdin);
                    if(open(node->input, O_RDONLY) == -1){
                        fprintf(stderr, "[%s] Bad input stream: %s", args[0], strerror(errno));
                        exit(1);
                    }
                }
                if(node->output != NULL){
                    fclose(stdout);
                    if(open(node->output, O_CREAT|O_WRONLY|O_TRUNC) == -1){
                        fprintf(stderr, "[%s] Bad output stream: %s", args[0], strerror(errno));
                        exit(1);
                    }
                }
                execvp(args[0], args);
                perror(args[0]);
                exit(1);                        
            } else {
                /* printf("%s (PID %d) started\n", args[0], pid); */
                if(!bg_run){
                    waitpid(pid, NULL, 0);
                    /* printf("%s (PID %d) finished\n", args[0], pid); */
                } else {
                    printf("%s [PID %d] start\n", args[0], pid);
                }
            }
        }
    }
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

    chain = parse_string(&bg_run, &status);
    print_chain(chain);

    if(say_if_error(status) == 0) {
        switch(look4cd(chain)){
            case 0:
                launch_chain(chain, bg_run);
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

/**
 * TODO: '|' convayor support
 */
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
    
    printf("[logout]\n");

    return 0;
}
