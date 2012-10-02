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

/**
* TODO: debug memory leak
* done?
*/
int main(int argc, const char * const * argv){
    struct node *list = NULL, *word;
    char *buffer;
    bool eof_err = false, space_as_letter = false;
    long int buffer_size, buffer_actual_size;
    long int last_quotes, pos;
    int ch;

    const char *hello = "root@8.8.8.8:~# ";
    const long int DEFAULT_BUFFER_LENGTH = 64;

    while(!eof_err){
        pos = 0;
        space_as_letter = false;
        word = (struct node*) malloc (sizeof(*word)); /* head word */
        word->data = NULL;
        word->next = NULL;
        list = word;
        buffer_actual_size = 0;
        buffer_size = DEFAULT_BUFFER_LENGTH;
        buffer = (char*) malloc (sizeof(char) * buffer_size);

        printf("%s", hello);
        do{
            ++pos;
            ch = getchar();
            if(ch == EOF){
                eof_err = true;
            }

            if(ch == '"'){
                space_as_letter ^= true;
                last_quotes = pos;
            } else
            if((ch == ' ' && !space_as_letter) || ch == EOF || ch == '\n'){
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
        } while(ch != '\n' && !eof_err);
        /* take structure to the screen */
        if(eof_err){
            /* we need to add one '\n' because there are no '\n''s in the string */
            putchar('\n');
        }
        if(!space_as_letter){
            for(word = list; word != NULL && word->data != NULL; word = word->next){
                printf("%s\n", word->data);
            }
        } else {
            printf("Missed quotes at position %ld\n", last_quotes);
        }
        /* free memory */
        destruct(list);
        free(buffer);
    }
    
    printf("[exit]\n");

    return 0;
}
