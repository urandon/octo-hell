#include <stdio.h>
#include <stdlib.h>

#define bool char
#define true 1
#define false 0

struct arg_list{
	struct node * start;
	long int count;
};

struct node{
    char *data;
    struct node *next;
};

void free_array(char** args)
{
	char** str;
	for(str = args; *str != NULL; str++){
		free(*str);
	}
	free(args);
}

void destruct_nodes(struct node *p)
{
	struct node *q;
	while(p != NULL){
		q = p->next;
		free(p);
		p = q;
	}
}

char** convert_list2array(struct node * list, long n)
{
	char ** a = (char **) malloc (sizeof(char *) * (n+1));
	struct node *p;
	long int i;
	
	for(i = 0, p = list; i < n; i++, p = p->next){
		a[i] = p->data;
	}
	a[n] = NULL;
	
	return a;
}


enum error_status {
    EOF_ERROR       = 1,
    BAD_QUOTES      = 2,
    UNKNOWN_ERROR   = 4,
};

char** parse_string(int * err_status, long * bad_quotes) /* returns status */
{
	char** args;
    struct node *list, *word, *prev_word = NULL;
    const long int DEFAULT_BUFFER_SIZE = 64;
    char *buffer = (char*) malloc (sizeof(char) * DEFAULT_BUFFER_SIZE);
    int eof_error = false, quotes_on = false;
    long int buffer_size = DEFAULT_BUFFER_SIZE, buffer_actual_size = 0;
    long int last_quotes, pos = 0;
    long int count;
    int ch;
    
    word = (struct node*) malloc (sizeof(*word)); /* head word */
    word->data = NULL;
    word->next = NULL;
    list = word;
    count = 0;

    do{
        ++pos;
        ch = getchar();
        if(ch == EOF){
            eof_error = true;
        }

        if(ch == '"'){
            quotes_on ^= true;
            last_quotes = pos;
        } else
        if((ch == ' ' && !quotes_on) || ch == EOF || ch == '\n'){
            /* drop buffer to structure if not empty */
            if(buffer_actual_size != 0){
				count++;
                long int i;
                word->data = (char*) malloc((buffer_actual_size + 1) * sizeof(char));
                for(i = 0; i < buffer_actual_size; i++){
                    word->data[i] = buffer[i];
                }
                word->data[buffer_actual_size] = '\0';
                word->next = (struct node *) malloc (sizeof(*word));
                prev_word = word;
                
                word = word->next;
                word->next = NULL;
                
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
    } while(ch != '\n' && !eof_error);

    /* free memory */
    free(buffer);
    /* check for empty string */
    free(word);
    if(prev_word == NULL){
		list = NULL;
	} else {
		prev_word->next = NULL;
	}
    
    *bad_quotes = (quotes_on) ? last_quotes : 0;
    *status = (eof_error * EOF_ERROR) | (quotes_on * BAD_QUOTES);
    /* return list; */
    args = convert_list2array(list, count);
    destruct_nodes(list);
    
    return args;
}


int main(int argc, const char * const * argv){
	char ** args;
    long bad_quotes_pos;
    int status = 0;

    const char *hello = "root@8.8.8.8:~# ";

    while(! (status & EOF_ERROR)){
        printf("%s", hello);
        args = parse_string(&status, &bad_quotes_pos);

        /* take structure to the screen */
        if(status & EOF_ERROR){
            /* we need to add one '\n' because there are no '\n''s in the string */
            putchar('\n');
        }
        if(status & BAD_QUOTES){
            printf("Missed quotes at position %ld\n", bad_quotes_pos);
        } else {
            char **str;
            for(str = args; *str != NULL; str++){
				printf("%s\n", *str);
			}
            
        }
        /* free memory */
        free_array(args);
    }
    
    printf("[logout]\n");

    return 0;
}
