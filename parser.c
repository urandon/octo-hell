#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "parser.h"

#define bool char
#define true 1
#define false 0

struct words{
    char *data;
    struct words *next;
};

void free_array(char** args)
{
    char** str;
    for(str = args; *str != NULL; str++){
        free(*str);
    }
    free(args);
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
