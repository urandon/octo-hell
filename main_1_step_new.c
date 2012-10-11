#include <stdio.h>
#include <stdlib.h>

#define bool char
#define true 1
#define false 0

struct node{
    char *data;
    struct node *next;
};

void destruct(struct node *p)
{
    struct node *q;
    while(p != NULL){
        q = p->next;
        free(p->data);
        free(p);
        p = q;
    }
}

char** convert_list2array();


enum error_status {
    EOF_ERROR       = 1,
    BAD_QUOTES      = 2,
    UNKNOWN_ERROR   = 4,
};    

struct node * parse_string(int * status, long * bad_quotes) /* returns status */
{
    struct node *list, *word;
    const long int DEFAULT_BUFFER_SIZE = 64;
    char *buffer = (char*) malloc (sizeof(char) * DEFAULT_BUFFER_SIZE);
    int eof_error = false, quotes_on = false;
    long int buffer_size = DEFAULT_BUFFER_SIZE, buffer_actual_size = 0;
    long int last_quotes, pos = 0;
    int ch;
    
    word = (struct node*) malloc (sizeof(*word)); /* head word */
    word->data = NULL;
    word->next = NULL;
    list = word;

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
                long int i;
                word->data = (char*) malloc((buffer_actual_size + 1) * sizeof(char));
                for(i = 0; i < buffer_actual_size; i++){
                    word->data[i] = buffer[i];
                }
                word->data[buffer_actual_size] = '\0';
                word->next = (struct node *) malloc (sizeof(*word));
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
    
    *bad_quotes = (quotes_on) ? last_quotes : 0;
    *status = (eof_error * EOF_ERROR) | (quotes_on * BAD_QUOTES);
    return list;
}

/**
* TODO: debug memory leak
*/
int main(int argc, const char * const * argv){
    struct node *list = NULL, *word;
    long bad_quotes_pos;
    int status = 0;

    const char *hello = "root@8.8.8.8:~# ";

    while(! (status & EOF_ERROR)){
        printf("%s", hello);
        list = parse_string(&status, &bad_quotes_pos);

        /* take structure to the screen */
        if(status & EOF_ERROR){
            /* we need to add one '\n' because there are no '\n''s in the string */
            putchar('\n');
        }
        if(status & BAD_QUOTES){
            printf("Missed quotes at position %ld\n", bad_quotes_pos);
        } else {
            for(word = list; word != NULL && word->data != NULL; word = word->next){
                printf("%s\n", word->data);
            }
        }
        /* free memory */
        destruct(list);
    }
    
    printf("[logout]\n");

    return 0;
}
