/* Linked list header file */

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "mysh.h"

/* Create a single node and link it to the node called 'n'. */
node * create_node(struct full_command * command, struct node * next);

/* Free all the nodes of a linked list. */
void free_list(node* list);

/* Print the elements of a list. */
void print_list(node* list);


#endif  /* LINKED_LIST_H */
