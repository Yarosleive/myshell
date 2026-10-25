#include "executor.h"
#include "builtins.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int execute(ExprNode *node, int bg) {
    if (node == NULL) {
        return 0;
    }
    switch (node->type) {
        case NODE_LIST:
            return execute_list(node, bg);
        case NODE_AND_OR:
            return execute_and_or(node, bg);
        case NODE_PIPELINE:
            return execute_pipeline(node, bg);
        case NODE_SIMPLE:
            return execute_simple(node, bg);
        case NODE_SUBSHELL:
            return execute_subshell(node, bg);
        default:
            perror("Executor: unknown ExprNode type");
            return 1;
    }
}

int apply_redirections(ExprNode *redirs) {
    ExprNode *cur = redirs;
    while (cur != NULL) {
        int target_fd;
        int flags = 0;
        if (cur->redir.op == REDIR_LESS) {
            target_fd = STDIN_FILENO;
            flags = O_RDONLY;
        } else if (cur->redir.op == REDIR_GREATER) {
            target_fd = STDOUT_FILENO;
            flags = O_WRONLY | O_CREAT | O_TRUNC;
        } else { // GG
            target_fd = STDOUT_FILENO;
            flags = O_WRONLY | O_CREAT | O_APPEND;
        }
        int fd = open(cur->redir.fname, flags, 0644);
        if (fd < 0) {
            perror("open");
            exit(1);
        }

        if (dup2(fd, target_fd) < 0) {
            perror("dup2");
            close(fd);
            exit(1);
        }
        close(fd);
        cur = cur->redir.next;
    }
    return 0;
}

int execute_list(ExprNode *node, int bg) {
    if (node == NULL) {
        return 0;
    }

    int bg2 = node->binary.op == OP_AMPERSAND;
    execute(node->binary.left, bg2 || bg);

    int status = 0;
    if (node->binary.right != NULL) {
        status = execute(node->binary.right, bg);
    }
    return status;
}

int execute_and_or(ExprNode *node, int bg) {
    if (node == NULL || node->binary.left == NULL || node->binary.right == NULL) {
        return 0;
    }
    int status = execute(node->binary.left, bg);

    if (node->binary.op == OP_AND_IF) {
        if (status == 0) {
            status = execute(node->binary.right, bg);
        }
        return status;
    }
    if (status != 0) {
        status = execute(node->binary.right, bg);
    }
    return status;
}

int execute_pipeline(ExprNode *node, int bg) {
    if (node == NULL) {
        return 0;
    }
    int pipefds[2];
    if (pipe(pipefds) < 0) {
        perror("pipe");
        exit(1);
    }
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        close(pipefds[0]);
        close(pipefds[1]);
        exit(1);
    } else if (pid1 == 0) {
        close(pipefds[0]);
        dup2(pipefds[1], STDOUT_FILENO);
        close(pipefds[1]);
        int status = execute(node->binary.left, 0);
        exit(status);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        close(pipefds[0]);
        close(pipefds[1]);
        exit(1);
    } else if (pid2 == 0) {
        close(pipefds[1]);
        dup2(pipefds[0], STDIN_FILENO);
        close(pipefds[0]);
        int status = execute(node->binary.right, 0);
        exit(status);
    }

    close(pipefds[0]);
    close(pipefds[1]);

    if (bg) {
        printf("[%d]\n", (int)pid2);
        return 0;
    }

    int status1, status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);

    if (WIFEXITED(status2)) {
        return WEXITSTATUS(status2);
    } else if (WIFSIGNALED(status2)) {
        return 128 + WTERMSIG(status2);
    }
    return 1;
}

int execute_simple(ExprNode *node, int bg) {
    if (node == NULL || node->simple.args == NULL || node->simple.args[0] == NULL) {
        return 0;
    }
    char **args = node->simple.args;
    int builtin_status = execute_builtin(args);
    if (builtin_status != -1) {
        return builtin_status;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        if (apply_redirections(node->simple.redirs) != 0) {
            perror("apply redirection");
            exit(1);
        }
        execvp(args[0], args);
        perror("exec");
        exit(127);
    }
    if (bg) {
        printf("[%d]\n", (int)pid);
        return 0;
    }
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return 1;
}

int execute_subshell(ExprNode *node, int bg) {
    if (node->subshell.inner == NULL) {
        return 0;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("pid");
        exit(1);
    } else if (pid == 0) {
        if (apply_redirections(node->subshell.redirs) != 0) {
            perror("apply redirection");
            exit(1);
        }
        int status = execute(node->subshell.inner, 0);
        exit(status);
    }
    
    if (bg) {
        printf("[%d]\n", (int)pid);
        return 0;
    }
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return 1;
}
