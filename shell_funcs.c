#include "shell_funcs.h"

// Builtin declarations (already in header, but implemented here)
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

// Builtin tables
char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit
};

int lsh_num_builtins(void) {
    return (int)(sizeof(builtin_str) / sizeof(builtin_str[0]));
}

// Builtin Function Implementations

int lsh_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("lsh");
        }
    }
    return 1;
}

int lsh_launch(char **args)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("lsh");
    } else {
        // Parent process: wait until child exits or is signaled
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int lsh_help(char **args) {
    (void)args; // silence unused-parameter warning
    printf("Omer Hayat's LSH\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built-in:\n");

    for (int i = 0; i < lsh_num_builtins(); i++) {
        printf(" %s\n", builtin_str[i]);
    }
    printf("Use the man command for information on other programs.\n");
    return 1;
}

int lsh_exit(char **args) {
    (void)args; // silence unused-parameter warning
    return 0;
}

int lsh_execute(char **args) {
    if (args[0] == NULL) {
        // An empty command was entered.
        return 1;
    }

    for (int i = 0; i < lsh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return lsh_launch(args);
}
