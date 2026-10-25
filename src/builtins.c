#include "builtins.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

Builtin builtins[] = {
    {"cd", builtin_cd},
    {"pwd", builtin_pwd},
    {"exit", builtin_exit},
    {"echo", builtin_echo},
    {NULL, NULL}
};

int execute_builtin(char **args) {
    if (args[0] == NULL) {
        return 0;
    }
    for (int i = 0; builtins[i].name != NULL; ++i) {
        if (strcmp(args[0], builtins[i].name) == 0) {
            return builtins[i].func(args);
        }
    }
    return -1;
}

int builtin_cd(char **args) {
    const char *path = args[1];
    if (path == NULL) {
        path = getenv("HOME");
        if (path == NULL) {
            perror("cd: HOME not set");
            return 1;
        }
    }
    if (chdir(path) != 0) {
        perror("cd");
        return 1;
    }
    return 0;
}

int builtin_exit(char **args) {
    int status = 0;
    if (args[1] != NULL) {
        status = atoi(args[1]);
    }
    exit(status);
}

int builtin_pwd(char **args) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    }
    perror("pwd");
    return 1;
}

int builtin_echo(char **args) {
    int i = 1;
    int flag = 0;
    if (args[1] != NULL && strcmp(args[1], "-n") == 0) {
        ++i;
        flag = 1;
    }
    while (args[i] != NULL) {
        printf("%s", args[i]);
        if (args[i + 1] != NULL) {
            printf(" ");
        }
        ++i;
    }
    if (!flag) {
        printf("\n");
    }
    return 0;
}