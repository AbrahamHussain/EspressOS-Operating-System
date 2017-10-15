/* Linked list header file */

#ifndef MYSH_H
#define MYSH_H

// This is a our struct for our commands
// COMMMENT MORE
typedef struct full_command {
    /* array of arguments */
    char ** arguments;
    /* number of arguments */
    int num_arguments;
    /* number of redirections */
    int num_redir;
    /* input file descriptor*/
    int input;
    /* output file descriptor*/
    int output;
    /* input file descriptor*/
    int error;
} full_command;

/* Node struct for linked list */
typedef struct node {
    struct full_command* command; /* Pointer to command struct */
    struct node * next; /* Pointer to next command */
} node;


void free_strings(char** str, int num_args);

node * create_command_node(struct full_command * command);

full_command * create_full_command(char ** args, int num_args);

node * create_linked_list(char ** args);

char** tokenize(char* str, int num_tokens);

char ** array_slicer(char ** arr, int start, int end);

int my_pipe(node * command_node);

void rem_redir(node* command_node);

int operator_processes (node * command_node);

int cdir_handler(char ** args);


#endif  /* MYSH_H */
