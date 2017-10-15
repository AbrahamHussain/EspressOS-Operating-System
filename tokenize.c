// File to test tokenization

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#define max_command_length 1024000 /* max length is 1000 KiB / 1 Byte */


/* pretty much a useless function but here for readability */
void read_line(char* str) {
    fgets(str, max_command_length, stdin);
}


char** tokenize(char* str, int num_tokens) {
    /* The regex to match with */
    char* r = "\"(([^\"\']+))\"|\'([^\"\']+)\'|([^[:space:]\"\'<|>]+)|<|>|\\|";
    /* Limit the amount of matches and groups to initialize array */
    int maxMatches = max_command_length;
    int maxGroups = 1;

    char** tokens = malloc(num_tokens * sizeof(char *));

    regex_t regex;
    /* The match array */
    regmatch_t matches[maxGroups];
    /* Iterator */
    unsigned int m;
    char* p;
    /* regcomp returns a nonzero number if it failed */
    if (regcomp(&regex, r, REG_EXTENDED)) {
        printf("Invalid regular expression\n");
        return NULL;
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
        char p_copy[strlen(p) + 1];
        strcpy(p_copy, p);
        /* Set end of substring to 0 in p_copy */
        p_copy[matches[0].rm_eo] = 0;
        /* Print the current match */
        printf("%s\n", p_copy + matches[0].rm_so);
        /* Store tokens */
        tokens[m] = malloc(strlen(p_copy + matches[0].rm_so) + 1);
        strcpy(tokens[m], p_copy + matches[0].rm_so);
        /* Move down the string by the size of the substring that matched */
        p += offset;
    }
    int f;
    for (f = 0; f < m; f++) {
        printf("%s\n", tokens[f]);
        // free(tokens[f]);
    }
    /* Free the regex that we compiled */
    regfree(&regex);
    return tokens;
}

/* Will eventually have to a main loop, loop until the exit code */
int main() {
    int num_tokens = 5;
    char** tokens = malloc(num_tokens * sizeof(char *));
    /* curr directory information for the user */
    printf("username:current/working/directory> ");

    char str[max_command_length];
    read_line(str);

    /* run tokenization */
    tokens = tokenize(str, num_tokens);
    int iter = 0;
    while (1) {
        if(tokens[iter] != NULL) {
            free(tokens[iter]);
            iter++;
        }
        else
            break;
    }
    return 0;
}
