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
        /* free(p->data);  old version. 1st stage. */
        free(p);
        p = q;
    }
}

void free_array(char ** a)
{
	char *p;
	for(p = *a; p != NULL; p++){
		free(p);
	}
	free(a);
}

char ** list2array(struct node *list, long int n)
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


int main(int argc, const char * const * argv){
    struct node *list = NULL, *word;
    char *buffer;
    long int buffer_size, buffer_actual_size;
    char **args;
    long int args_number;
    bool eof_err = false, space_as_letter = false;
    long int last_quotes_position, position;
    int ch;

    const char *hello = "root@8.8.8.8:~# ";
    const long int DEFAULT_BUFFER_LENGTH = 64;

    while(!eof_err){
		/* initiate for new input */
        position = 0;
        space_as_letter = false;
        word = (struct node*) malloc (sizeof(*word)); /* head word */
        word->data = NULL;
        word->next = NULL;
        list = word;
        
        args_number = 0;
        args = NULL;
        buffer_actual_size = 0;
        buffer_size = DEFAULT_BUFFER_LENGTH;
        buffer = (char*) malloc (sizeof(char) * buffer_size);

        printf("%s", hello);
        /* get string and build structure */
        do{
            ++position;
            ch = getchar();
            if(ch == EOF){
                eof_err = true;
            }

            if(ch == '"'){
                space_as_letter ^= true;
                last_quotes_position = position;
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
                    
                    args_number++;
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
        free(buffer);
        
        /* 2nd step: convert list to array */
        args = list2array(list, args_number);
        destruct(list);
        
        /* take structure to the stream */
        if(eof_err){
            /* we need to add one '\n' because there are no '\n''s in the string */
            putchar('\n');
        }
        if(!space_as_letter){
			char * arg;
			int i;
            for(arg = *args, i = 0; i < args_number && arg != NULL; arg++, i++){
                printf("%s\n", arg);
            }
        } else {
            printf("Unmatched quotes at position %ld\n", last_quotes_position);
        }
        
        
        /* free memory */
        free_array(args);
        
    }
    
    printf("[exit]\n");

    return 0;
}
