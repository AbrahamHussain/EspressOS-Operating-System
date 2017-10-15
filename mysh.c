#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <limits.h> // get max path
#include <pwd.h> // for userlogin
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <readline/readline.h>
#include <readline/history.h>
#include "mysh.h"
#include "linked_list.h"
#include  <sys/types.h>
#include <sys/wait.h>


#define max_command_length 1024000 /* max length is 1000 KiB / 1 Byte */


/* free an array of strings */
void free_strings(char** str, int num_args) {
    /* Iterator i */
    int i;
    for (i = 0; i < num_args; i++) {
        /* Free each string in a command */
        free(str[i]);
    }
    /* Free the actual array */
    free(str);
}


/* Remove element of array */
void remove_element(char * string, int index, int array_length) {
    int i;
    for (i = index; i < array_length - 1; i++) {
        string[i] = string[i + 1];
    }
}


/* Creates a linked list node with a command stored in it */
node * create_command_node(struct full_command * command) {
    /* New node contains the a full_command and next node, NULL */
    node * new_node = create_node(command, NULL);
    return new_node;
}


/* Initalizes a full_command */
full_command * create_full_command(char ** args, int num_args) {
    /* Allocate memory for a new full command */
    full_command * new_full_command = (full_command *) malloc(sizeof(full_command));
    /* error handling for command creation */
    if (new_full_command == NULL) {
        fprintf(stderr, "Could not allocate memory for new full_command!");
        exit(1);
    }

    /* Allocate memory for a full command */
    new_full_command->arguments = (char**) malloc(sizeof(char *) * num_args);
    /* error handling for argument space allocation */
    if (new_full_command->arguments == NULL) {
        fprintf(stderr, "Could not allocate memory for arguments!");
        exit(1);
    }

    /* Set arguments */
    new_full_command->arguments = args;
    /* Set size of current command */
    new_full_command->num_arguments = num_args;
    /* Initalize input, output, and error file descriptors */
    new_full_command->input = -1;
    new_full_command->output = -1;
    new_full_command->error = -1;
    /* Couns the number of redirections within a command */
    new_full_command->num_redir = 0;
    return new_full_command;
}


/* Creates a linked list out of a tokenized arr of arguments
 * Argument(s):
 *              args: Tokenized command in an array of strings
 * Output:
 *              res->next: A pointer to the head of the linked list
 */
node * create_linked_list(char ** args) {
    /* These are going to be pointers that we use for our two pointer method */
    /* i points to the beginning of the current command */
    int i = 0;
    /* j points to the current argument that we're iterating on */
    int j = 0;
    /* iterator for number of arguments */
    int num_args = 0;

    /* Iterator node */
    node * iterator_node = create_command_node(NULL);
    /* Result node, always points to head of linked list */
    node * res = iterator_node;

    /* Loop */
    while(1) {
        /* If current argument is NULL or a pipe, wrap up current command */
        if ((args[j] == NULL) || (strcmp(args[j], "|") == 0)) {
            /* Initialize a temporary array of strings */
            char ** temp;
            /* Slice our command list to only what the current command should be */
            temp = array_slicer(args, i, j);

            /* Create a command with the temp array */
            full_command * new_command = create_full_command(temp, num_args);

            /* Create a node that contains the new command */
            node * new_node = create_command_node(new_command);
            /* Update our node in order to keep adding */
            iterator_node->next = new_node;
            iterator_node = iterator_node->next;

            /* If the argument is a Null argument, return the linked list */
            if(args[j] == NULL) {
                return res->next;
            }
            /* Move j past pipe, set i to beginning of next substring */
            j++;
            i = j;
            /* Reset number of arguments */
            num_args = 0;
        }
        else {
            /* Iterate the current argument, and add 1 to count of arguments */
            num_args++;
            j++;
        }
    }

    return res->next;
}


/* Tokenizing function
 * Arguments:
 *          str: A string to tokenize, result of line read from shell
 *          num_tokens: base number of tokens to initialize size of arr
 * Output:
 *          Array of strings, each a token from the original string
 */
char** tokenize(char* str, int num_tokens) {
    /* The regex that tokenizes the string */
    char* r = "\"(([^\"\']+))\"|\'([^\"\']+)\'|([^[:space:]\"\'<|>]+)|<|>|\\|";
    /* Limit the amount of matches and groups to initialize array */
    int maxMatches = max_command_length;
    int maxGroups = 1;

    /* Initialize tokens array */
    char** tokens = malloc(num_tokens * sizeof(char *));

    if (tokens == NULL) {
        fprintf(stderr, "Could not tokenize the command arguments!");
        /* Invalid tokenizng */
        exit(1);
    }

    regex_t regex;
    /* The match array */
    regmatch_t matches[maxGroups];
    /* Iterator */
    unsigned int m;
    char* p;
    /* regcomp returns a nonzero number if it failed */
    if (regcomp(&regex, r, REG_EXTENDED)) {
        fprintf(stderr, "Invalid regular expression");
        exit(1);
    }
    /* Iterators */
    m = 0;
    p = str;

    for (m = 0; m < maxMatches; m++) {
        /* Reallocate if m is bigger than the size of the array */
        if (m == num_tokens) {
            num_tokens *= 2;
            tokens = realloc(tokens, (num_tokens * sizeof(char *)));
        }
        if (tokens == NULL) {
            fprintf(stderr, "Could not tokenize the command arguments!");
            /* Invalid tokenizng */
            exit(1);
        }

        /* If no more matches we just break out of the loop */
        if (regexec(&regex, p, maxGroups, matches, 0)) {
            break;
        }
        /* Offset of the match in the string */
        unsigned int offset = 0;
        if (matches[0].rm_so == (size_t)-1) {
            break;
        }
        /* Move offset to end of the current match */
        offset = matches[0].rm_eo;
        /* Copy string from beginning of current match */
        char* p_copy = malloc(sizeof(char) * (strlen(p) + 1));
        strcpy(p_copy, p);
        /* Set end of substring to 0 in p_copy */
        p_copy[matches[0].rm_eo] = 0;
        /* If first char is a quote, do not add to command */
        if ((p[1] == '\"') || p[1] == '\'') {
            /* Size of array currently */
            int array_length = strlen(p_copy + matches[0].rm_so) + 1;
            /* Set second to last index of current substring to 0 */
            p_copy[matches[0].rm_eo - 1] = 0;
            /* Remove the quote from the beginning of the string */
            remove_element(p_copy, 0, array_length);
            /* Reallocate size of string */
            char* tmp = realloc(p_copy, (array_length - 1) * sizeof(char));
            if (tmp == NULL) {
                fprintf(stderr, "Could not allocate space for stripped quotes");
                exit(1);
            }
            p_copy = tmp;
            /* store tmp */
            tokens[m] = malloc(strlen(tmp + matches[0].rm_so) + 1);
            strcpy(tokens[m], tmp + matches[0].rm_so);

        }
        else {
            tokens[m] = malloc(strlen(p_copy + matches[0].rm_so) + 1);
            if (tokens[m] == NULL) {
                fprintf(stderr, "Could not allocate space for one token");
                exit(1);
            }
            strcpy(tokens[m], p_copy + matches[0].rm_so);
        }
        /* Store tokens */

        /* Move down the string by the size of the substring that matched */
        p += offset;
    }
    /* tokens array should end with NULL */
    tokens[m + 1] = NULL;
    /* Free the regex that we compiled */
    regfree(&regex);
    return tokens;
}


/* Slices an array of strings to get just the strings between start and end */
char ** array_slicer(char ** arr, int start, int end) {
    /* Allocate memory for new array */
    char ** sliced = malloc(sizeof(char **) * (end - start + 1));
    /* If sliced's memory was not allocated correctly */
    if (sliced == NULL) {
        fprintf(stderr, "Could not allocate memory for sliced subcommand!");
        exit(1);
    }
    /* Iterate through the passed in arr to slice it */
    int i;
    int j = 0;
    for(i = start; i < end; ++i) {
        sliced[j] = arr[i];
        j++;
    }
    return sliced;
}


/* my_pipe: pipes and sets file descriptors between commands
 * Argument(s):
 *              command_node: head of linked list of commands
 * Output: void
 */
int my_pipe(node * command_node) {
    /* Running pointer that will iterate through linked list of command */
    node* iterator_node = command_node;
    int at_head = 1;
    /* next node will be NULL when we are at tail of linked list */
    while(iterator_node != NULL) {
        /* The command inside of the current node */
        full_command* curr_cmd = iterator_node->command;

        /* if at head, initalize input to stdin */
        if (at_head) {
            curr_cmd->input = STDIN_FILENO;
            at_head = 0;
        }
        /* if at tail, initialize output to stdout */
        if (iterator_node->next == NULL) {
            curr_cmd->output = STDOUT_FILENO;
        }
        /* Input and output should be set with pipe. Set up
         * filedes for adjacent commands as initial directions
         */
        /* Connect ouput of current command to input of next command */
        if (iterator_node->next != NULL) {
            int filedes[2];
            /* Set up filedes piping */
            pipe(filedes);
            /* set output of current command */
            curr_cmd->output = filedes[1];
            /* Set input of next command */
            iterator_node->next->command->input = filedes[0];
        }

        /* Count size of command without redirects and next string */
        int i;

        /* iterate through current command to set redirection */
        for (i = 0; i < curr_cmd->num_arguments; i++) {
            /* Check for for input redirection inside of current command */
            if (strcmp(curr_cmd->arguments[i], "<") == 0) {
                /* Next argument must be a valid file,
                 * open will return a negative value if it failed
                 */
                int fd_in = open(curr_cmd->arguments[i + 1], O_RDONLY);
                if (fd_in < 0) {
                    printf("%s: Not a valid file\n", curr_cmd->arguments[i + 1]);
                    return -1;
                }
                else {
                    /* close filedes so it can be used in the future */
                    if (curr_cmd->input != STDIN_FILENO) {
                        close(curr_cmd->input);
                    }
                    /* set output of current command to filedes */
                    curr_cmd->input = fd_in;
                }
                /* skip over the next element in arguments */
                i++;
            }

            /* Check for output redirection inside of current command */
            else if (strcmp(curr_cmd->arguments[i], ">") == 0) {
                /* Next argument must be a valid file,
                 * open will return a negative value if it failed
                 */
                /* Handle truncation here */
                int fd_out;
                if (strcmp(curr_cmd->arguments[i + 1], ">") == 0) {
                    fd_out = open(curr_cmd->arguments[i + 2],\
                        O_CREAT|O_WRONLY|O_APPEND, S_IRUSR|S_IWUSR);
                }
                else {
                    fd_out = open(curr_cmd->arguments[i + 1],\
                        O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
                }
                if (fd_out < 0) {
                    printf("%s: Not a valid file\n", curr_cmd->arguments[i + 1]);
                    return -1;
                }
                else {
                    /* We should not close STDOUT */
                    if (curr_cmd->output != STDOUT_FILENO) {
                        close(curr_cmd->output);
                    }
                    /* set output of current command to filedes */
                    curr_cmd->output = fd_out;
                }

                /* If next is another redirect ouput, then we want to skip it too */
                if (strcmp(curr_cmd->arguments[i], ">") == 0) {
                    i += 2;
                }
                else {
                    /* Skip over to next element in the arguments */
                    i++;
                }
            }
        }
        /* iterate to next node */
        iterator_node = iterator_node->next;
    }
    return 0;
}


/* rem_redir: removes redirection clauses from command
 * Argument(s):
 *              command_node: linked list of commands, should be piped
 * Output: void
 */
void rem_redir(node* command_node) {
    node* iterator_node = command_node;

    while (iterator_node != NULL) {
        /* The command inside of the current node */
        full_command* curr_cmd = iterator_node->command;
        int k;
        int new_args_iter = 0;
        /* Initialize new arguments array, with redirections removed */
        char** new_args = (char**) malloc(sizeof(char*) *\
            (curr_cmd->num_arguments - curr_cmd->num_redir + 1));
        if (new_args == NULL){
            fprintf(stderr, "Could not allocate space to new_args");
            exit(1);
        }

        /* Iterate through current command and get rid of redirections */
        for (k = 0; k < curr_cmd->num_arguments; k++) {
            /* If not a redirect we just add it back to the command */
            if(!((strcmp(curr_cmd->arguments[k], ">") == 0) ||\
                (strcmp(curr_cmd->arguments[k], "<") == 0))) {
                /* allocate memory for the new arguments */
                new_args[new_args_iter] = (char* ) malloc(sizeof(char) *\
                    strlen(curr_cmd->arguments[k]));
                /* Error handling in case allocation failed */
                if (new_args[new_args_iter] == NULL) {
                    fprintf(stderr, "Could not allocate space for argument \n");
                    exit(1);
                }
                /* Set new argument */
                new_args[new_args_iter] = curr_cmd->arguments[k];
                new_args_iter++;
            }
            else {
                /* redirect and the next file should not be in command after */
                curr_cmd->num_redir += 2;
                if ((strcmp(curr_cmd->arguments[k], ">") == 0) &&\
                    (strcmp(curr_cmd->arguments[k+1], ">") == 0)) {
                    k++;
                    curr_cmd->num_redir++;
                }
                k++;
            }
        }
        /* End arguments with NULL */
        new_args[new_args_iter] = NULL;
        /* Set arguments to the new arguments */
        curr_cmd->arguments = new_args;
        /* Change the number of arguments */
        curr_cmd->num_arguments -= curr_cmd->num_redir;

        /* Iterate to next command node */
        iterator_node = iterator_node->next;
    }
    /* Free iterator */
    free_list(iterator_node);
}


/* This function is going to take in the operator (e.g. grep, sort, etc)
   and as well as the list of arguements if there are any
   Operator must be not NULL but arsg can be potentially NULL
*/
int operator_processes (node * command_node) {
    /* iterative node */
    node* iterator_node = command_node;

    int died = 0;

    while ((iterator_node != NULL) && (died == 0)) {
        /* current command struct */
        full_command* curr_cmd = iterator_node->command;
        /* current command */
        char * operator = curr_cmd->arguments[0];
        /* current arguments */
        char ** args = curr_cmd->arguments;
        /* current input file descriptor */
        int fd_in = curr_cmd->input;
        /* current output file descriptor */
        int fd_out = curr_cmd->output;

        /* This will be the id of the child process */
        pid_t child_pid, pid;
        int status;
        /* get parent id */
        pid = getppid();
        /* get child id and fork */
        child_pid = fork();

        /* This case is when the child process has failed */
        if(child_pid < 0) {
            printf("Error: invalid command");
            fprintf(stderr, "fork failed!");
            return -1;
        /* This is the case when the child has succceeded */
        }
        else if (child_pid == 0) {
            /* Set pointer to input file descriptor */
            dup2(fd_in, STDIN_FILENO);
            /* Set pointer to output file descriptor */
            dup2(fd_out, STDOUT_FILENO);

            /* This is going to execute the commands based on
             * the initial operator and its arguements */
            if (execvp(operator, args) < 0) {
                printf("Invalid command!\n");
            }
            exit(1);

        /* in this case we are going to use the parent process */
        }
        else if (child_pid > 0) {
            /* We are going to check if the waiting was unsuccessful */
            if (pid < 0) {
                perror("waiting was unsuccessful...\n");
                return -1;
            }
            /* This is going to wait for the child process to finish */
            pid = wait(&status);
            // free(curr_cmd->arguments);
            /* Do not close stdin */
            if (fd_in != STDIN_FILENO) {
                close(fd_in);
            }
            /* Do not close stdout */
            if (fd_out != STDOUT_FILENO) {
                close(fd_out);
            }
        }
        /* iterate the node */
        iterator_node = iterator_node->next;
    }
    /* free the iterative node */
    free_list(iterator_node);
    return 0;
}


/* Change Directory handler */
int cdir_handler(char ** args) {
    /* If there is no second argument, CD will not do anything */
    if (args[1] == NULL) {
        fprintf(stderr, "expected an argument: no argument");
        return -1;
    }
    /* Perform cd and see if it works */
    else {
    /* chdir fails if it returns -1 */
        if (chdir(args[1]) == -1) {
            perror("Directory does not exist");
            return 1;
        }
    }
    return 0;
}

/* Main loop for command shell */
int main() {
    /* Run until exit */
    while(1) {
        /* Base number of tokens to initialize tokens array */
        int num_tokens = 1000;
        /* Malloc tokens to allocate memory */
        char** tokens = malloc(num_tokens * sizeof(char *));
        if (tokens == NULL) {
            fprintf(stderr, "Could not allocate space for tokens\n");
        }

        /* Get name of current user */
        char *name;
        struct passwd *pass;
        pass = getpwuid(getuid());
        name = pass->pw_name;

        /* Get the path to current directory */
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL)
            perror("getcwd() error");

        /* User and current working directory the command shell
         * num tokens was used because we already stored it as a base size
         */
        char * user_dir = malloc(sizeof(char) * num_tokens);
        if (user_dir == NULL) {
            fprintf(stderr, "Could not allocate space for current directory");
        }

        /* Add user:directory information */
        sprintf(user_dir, "%s:%s> ", name, cwd);
        /* readline allows us to use the arrow keys */
        char * str = readline(user_dir);
        /* Get previous commands by going up and down with arrow keys */
        add_history(str);

        /* Tokenize the arguments */
        tokens = tokenize(str, num_tokens);
        int s = 0;
        while(tokens[s] != NULL) {
            s++;
        }

        /* If the first token is change directory, run cd handler */
        if ((strcmp(tokens[0], "cd") == 0)) {
            cdir_handler(tokens);
            continue;
        }
        /* If it is an exit command, free tokens and exit successfully */
        if ((strcmp(tokens[0], "exit")) == 0) {
            exit(0);
        }
        /* Make a new linked list from the tokens retrieved from cmd line */
        node* linked_list1 = create_linked_list(tokens);
        /* perform piping */
        int k = my_pipe(linked_list1);
        if (k == -1) {
            continue;
        }
        /* remove redirection from commands */
        rem_redir(linked_list1);
        /* perform commands with series of forks */
        int f = operator_processes(linked_list1);
        if (f == -1) {
            continue;
        }
        /* Free user_dir string */
        free(user_dir);
        /* Free the linked list */
        free_list(linked_list1);
    }
    return 0;
}
