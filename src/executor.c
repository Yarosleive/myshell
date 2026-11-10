#include "executor.h"
#include "builtins.h"
#include "jobs.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_PIPE_LEN 256

int is_builtin(char *name) {
    return (strcmp("echo", name) == 0 || strcmp("cd", name) == 0 || strcmp("pwd", name) == 0 ||
            strcmp("help", name) == 0 || strcmp("exit", name) == 0 || strcmp("jobs", name) == 0 ||
            strcmp("fg", name) == 0 || strcmp("bg", name) == 0 || strcmp("kill", name) == 0 ||
            strcmp("wait", name) == 0);
}

void set_signals_on_default(void) {
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
}

void check_if_stopped(int job_id, int status1, int status2) {
    Job *job = find_job_by_id(job_id);
    if (job != NULL) {
        if (WIFSTOPPED(status1) || WIFSTOPPED(status2)) {
            job->state = JOB_STOPPED;
            printf("\n[%d]+  Stopped\t\t%s\n", job->id, job->command);
        } else {
            remove_job_by_id(job_id);
        }
    }
}

int apply_redirections(ExprNode *redirs) {
    ExprNode *cur = redirs;
    while (cur != NULL) {
        if (cur->type != NODE_REDIR) {
            break;
        }

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
            fprintf(stderr, "apply redir: open\n");
            exit(1);
        }

        if (dup2(fd, target_fd) < 0) {
            fprintf(stderr, "apply redir: dup2\n");
            close(fd);
            exit(1);
        }
        close(fd);
        cur = cur->redir.next;
    }
    return 0;
}

void launch_command(ExprNode *node) {
    if (node == NULL || node->simple.args == NULL || node->simple.args[0] == NULL) {
        exit(0);
    }
    char **args = node->simple.args;

    if (args == NULL || args[0] == NULL) {
        exit(0);
    }

    if (apply_redirections(node->simple.redirs) != 0) {
        fprintf(stderr, "apply redirection");
        exit(1);
    }

    int builtin_status = execute_builtin(args);
    if (builtin_status != -1) {
        exit(builtin_status);
    }

    execvp(args[0], args);
    fprintf(stderr, "mysh: %s: command not found\n", args[0]);
    exit(127);
}

int execute(ExprNode *node, int bg, char *cmd_line) {
    if (node == NULL) {
        return 0;
    }
    switch (node->type) {
        case NODE_LIST:
            return execute_list(node, bg, cmd_line);
        case NODE_AND_OR:
            return execute_and_or(node, bg, cmd_line);
        case NODE_PIPELINE:
            return execute_pipeline(node, bg, cmd_line);
        case NODE_SIMPLE:
            return execute_simple(node, bg, cmd_line);
        case NODE_SUBSHELL:
            return execute_subshell(node, bg, cmd_line);
        default:
            fprintf(stderr, "Executor: unknown ExprNode type\n");
            return 1;
    }
}

int execute_list(ExprNode *node, int bg, char *cmd_line) {
    if (node == NULL) {
        return 0;
    }

    int bg2 = node->binary.op == OP_AMPERSAND;
    execute(node->binary.left, bg2 || bg, cmd_line);

    int status = 0;
    if (node->binary.right != NULL) {
        status = execute(node->binary.right, bg, cmd_line);
    }
    return status;
}

int execute_and_or(ExprNode *node, int bg, char *cmd_line) {
    if (node == NULL || node->binary.left == NULL || node->binary.right == NULL) {
        return 0;
    }
    int status = execute(node->binary.left, bg, cmd_line);

    if (node->binary.op == OP_AND_IF) {
        if (status == 0) {
            status = execute(node->binary.right, bg, cmd_line);
        }
        return status;
    }
    if (status != 0) {
        status = execute(node->binary.right, bg, cmd_line);
    }
    return status;
}

void collect_pipeline(ExprNode *node, ExprNode **cmds, int *count) {
    if (node == NULL) {
        return;
    }
    if (node->type == NODE_PIPELINE && node->binary.op == OP_PIPE) {
        collect_pipeline(node->binary.left, cmds, count);
        collect_pipeline(node->binary.right, cmds, count);
    } else if (node != NULL) {
        cmds[(*count)++] = node;
    }
}

int execute_pipeline(ExprNode *node, int bg, char *cmd_line) {
    if (node == NULL) {
        return 0;
    }
    
    ExprNode *cmds[MAX_PIPE_LEN];
    int count = 0;
    collect_pipeline(node, cmds, &count);

    if (count <= 0) {
        return 0;
    } else if (count == 1) {
        return execute(cmds[0], bg, cmd_line);
    }

    int num_pipes = count - 1;
    int pipefds[num_pipes][2];

    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipefds[i]) < 0) {
            fprintf(stderr, "execute_pipeline: pipe\n");
            exit(1);
        }
    }

    pid_t pids[256];
    pid_t pgid = 0;

    for (int i = 0; i < count; ++i) {
        pid_t pid = fork();

        if (pid < 0) {
            fprintf(stderr, "Execute_pipeline: fork\n");
            exit(1);
        } else if (pid == 0) {
            if (i == 0) {
                pgid = getpid();
            }
            setpgid(0, pgid);

            set_signals_on_default();

            if (i > 0) {
                if (dup2(pipefds[i - 1][0], STDIN_FILENO) < 0) {
                    fprintf(stderr, "Execute_pipeline: dup2\n");
                    exit(1);
                }
            }

            if (i < num_pipes) {
                if (dup2(pipefds[i][1], STDOUT_FILENO) < 0) {
                    fprintf(stderr, "Execute_pipeline: dup2\n");
                    exit(1);
                }
            }

            for (int j = 0; j < num_pipes; ++j) {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }

            if (cmds[i] == NULL) {
                continue;
            } else if (cmds[i]->type == NODE_SIMPLE) {
                if (apply_redirections(cmds[i]->simple.redirs) != 0) {
                    exit(1);
                }
                char **args = cmds[i]->simple.args;
                int builtin_status = execute_builtin(args);
                if (builtin_status != -1) {
                    exit(builtin_status);
                }
                execvp(args[0], args);
                fprintf(stderr, "mysh: %s: command not found\n", args[0]);
                exit(127);
            } else if (cmds[i]->type == NODE_SUBSHELL) {
                int status = execute(cmds[i], 0, cmd_line);
                exit(status);
            } else {
                fprintf(stderr, "Error: unexpected node type %d in pipeline\n", cmds[i]->type);
                exit(1);
            }
        } else {
            if (i == 0) {
                pgid = pid;
            }
            setpgid(pid, pgid);
            pids[i] = pid;
        }
    }

    for (int i = 0; i < num_pipes; i++) {
        close(pipefds[i][0]);
        close(pipefds[i][1]);
    }

    int job_id = add_job(pgid, cmd_line);

    if (bg) {
        printf("[%d] %d\n", job_id, (int)pids[count - 1]);
        return 0;
    }

    tcsetpgrp(STDIN_FILENO, pgid);

    int statuses[MAX_PIPE_LEN];
    int stopped = 0;

    for (int i = 0; i < count; i++) {
        waitpid(pids[i], &statuses[i], WUNTRACED);
        if (WIFSTOPPED(statuses[i])) {
            stopped = 1;
        }
    }

    tcsetpgrp(STDIN_FILENO, getpgrp());

    Job *job = find_job_by_id(job_id);
    if (job != NULL) {
        if (stopped) {
            job->state = JOB_STOPPED;
            printf("\n[%d]+  Stopped\t\t%s\n", job->id, job->command);
        } else {
            remove_job_by_id(job_id);
        }
    }

    int last_status = statuses[count - 1];
    if (WIFEXITED(last_status)) {
        return WEXITSTATUS(last_status);
    } else if (WIFSIGNALED(last_status)) {
        return 128 + WTERMSIG(last_status);
    }
    return 1;
}

int execute_simple(ExprNode *node, int bg, char *cmd_line) {
    if (node == NULL || node->simple.args == NULL || node->simple.args[0] == NULL) {
        return 0;
    }
    char **args = node->simple.args;

    if (bg == 0 && is_builtin(args[0])) {
        if (strcmp(args[0], "cd") == 0) {
            return builtin_cd(args);
        }

        int saved_stdout = dup(STDOUT_FILENO);
        int saved_stdin = dup(STDIN_FILENO);

        if (apply_redirections(node->simple.redirs) != 0) {
            dup2(saved_stdout, STDOUT_FILENO);
            dup2(saved_stdin, STDIN_FILENO);
            close(saved_stdout);
            close(saved_stdin);
            return 1;
        }

        int status = execute_builtin(args);

        fflush(stdout);
        dup2(saved_stdout, STDOUT_FILENO);
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdout);
        close(saved_stdin);

        return status;
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "execute_simple: fork\n");
        exit(1);
    } else if (pid == 0) {
        setpgid(0, 0);

        set_signals_on_default();

        if (apply_redirections(node->simple.redirs) != 0) {
            fprintf(stderr, "apply redirection");
            exit(1);
        }

        if (bg) {
            int builtin_status = execute_builtin(args);
            if (builtin_status != -1) {
                return builtin_status;
            }
        }

        execvp(args[0], args);
        fprintf(stderr, "mysh: %s: command not found\n", args[0]);
        exit(127);
    }

    setpgid(pid, pid);
    int job_id = add_job(pid, cmd_line);

    if (bg) {
        printf("[%d] %d\n", job_id, (int)pid);
        return 0;
    }

    tcsetpgrp(STDIN_FILENO, pid);

    int status;
    waitpid(pid, &status, WUNTRACED);

    tcsetpgrp(STDIN_FILENO, getpgrp());

    check_if_stopped(job_id, status, 0);

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return 1;
}

int execute_subshell(ExprNode *node, int bg, char *cmd_line) {
    if (node->subshell.inner == NULL) {
        return 0;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("pid");
        exit(1);
    } else if (pid == 0) {
        setpgid(0, 0);

        set_signals_on_default();

        if (apply_redirections(node->subshell.redirs) != 0) {
            fprintf(stderr, "apply redirection\n");
            exit(1);
        }
        int status = execute(node->subshell.inner, 0, cmd_line);
        exit(status);
    }
    
    setpgid(pid, pid);
    int job_id = add_job(pid, cmd_line);

    if (bg) {
        printf("[%d] %d\n", job_id, (int)pid);
        return 0;
    }

    tcsetpgrp(STDIN_FILENO, pid);

    int status;
    waitpid(pid, &status, WUNTRACED);

    tcsetpgrp(STDIN_FILENO, getpgrp());

    check_if_stopped(job_id, status, 0);

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return 1;
}
