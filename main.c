#include "shell_funcs.h"

#define LSH_RL_BUFSIZE 1024
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

char **lsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof *tokens);
    char *token;

    if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position++] = token;

        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            char **tmp = realloc(tokens, bufsize * sizeof *tokens);
            if (!tmp) {
                free(tokens);
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
            tokens = tmp;
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

char *lsh_read_line(void)
{
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        c = getchar();

        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        }

        buffer[position++] = (char)c;

        if (position >= bufsize) {
            bufsize += LSH_RL_BUFSIZE;
            char *tmp = realloc(buffer, bufsize);
            if (!tmp) {
                free(buffer);
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
            buffer = tmp;
        }
    }
}

void lsh_loop(void) {
    char *line;
    char **args;
    int status;

    do {
        printf("> ");
        line = lsh_read_line();
        args = lsh_split_line(line);
        status = lsh_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    lsh_loop();
    return EXIT_SUCCESS;
}
