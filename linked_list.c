/* Linked list implementation */

#include <stdio.h>
#include <stdlib.h>
#include "linked_list.h"

/* Makes a new node from a command and connects it to next node */
node * create_node(struct full_command * command, struct node * next) {
    /* Allocate memory for new node of linked list */
    node* new_node = (node *)malloc(sizeof(node));
    /* Error checking */
    if (new_node == NULL) {
        fprintf(stderr, "Fatal error: out of memory. "
                "Terminating program...\n");
        exit(1);
    }

    /* Initialize contents of the node */
    new_node->command = command;
    new_node->next = next;

    return new_node;
}

/* Frees a linked list */
void free_list(node *list) {
    node* n; /* Command node */

    while (list != NULL) {
        /* Set pointer to beginning */
        n = list;
        /* Move the list down to next node */
        list = list->next;
        /* Free the current node */
        free(n);
    }
}

/* Prints out the nodes of a linked list */
void print_list(node* list) {
    node *n = list;
    while (n != NULL) {
        int i;
        for (i = 0; i < n->command->num_arguments; i++) {
            /* New line to delimit separate commands */
            if (i == n->command->num_arguments - 1) {
                printf("%s\n", n->command->arguments[i]);
            }
            else{
                printf("%s", n->command->arguments[i]);
            }
        }
        /* iterate to next node */
        n = n->next;
    }
    /* Free the actual node */
    free_list(n);
}