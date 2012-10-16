#ifndef PARSER_H
#define RARSER_H

enum error_status{
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

void destruct_chain(struct exec_node *p);
struct exec_node * parse_string(int * bg_run, int * err_status);

#endif
