#pragma once
#include <stdlib.h>

typedef int (*builtit_func_t)(char **args);

typedef struct {
    char *name;
    builtit_func_t func;
} Builtin;

int builtin_cd(char **args);
int builtin_exit(char **args);
int builtin_pwd(char ** args);
int builtin_echo(char **args);

extern Builtin builtins[];

int execute_builtin(char **args);
