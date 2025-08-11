#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#ifndef LSH_H
#define LSH_H

// Builtin shell command declarations
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

// Utility functions
int lsh_num_builtins(void);
int lsh_execute(char **args);
int lsh_launch(char **args); // Declared here, but defined elsewhere

#endif /* LSH_H */
