#include "builtins.h"
#include "jobs.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

Builtin builtins[] = {
    {"cd", builtin_cd},
    {"pwd", builtin_pwd},
    {"exit", builtin_exit},
    {"echo", builtin_echo},
    {"help", builtin_echo},
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

int builtin_help(char **args) {
    puts("| Команда | Назначение |");
    puts("|---------|-----------|");
    puts("| `cd [каталог]` | смена текущего каталога |");
    puts("| `pwd` | вывод текущего каталога |");
    puts("| `echo [-n] [аргументы]` | вывод аргументов |");
    puts("| `help` | краткая справка по встроенным командам |");
    puts("| `exit [код]` | завершение работы интерпретатора |");
    puts("| `jobs` | список заданий |");
    puts("| `fg [%n]` | перевод задания на передний план |");
    puts("| `bg [%n]` | продолжение остановленного задания в фоне |");
    puts("| `kill` | отправка сигнала процессу или заданию |");
    puts("| `wait [pid\|%n]` | ожидание завершения заданий |");
    return 1;
}

int builtin_jobs(char **args) {
    update_jobs();
    for (int i = 0; i < MAX_JOBS; ++i) {
        Job *job = get_job_by_ind(i);
        if (job != NULL && job->state != JOB_DONE) {
            char marker;
            if (job == get_cur_job()) {
                marker = '+';
            } else if (job == get_prev_job()) {
                marker = '-';
            } else {
                marker = ' ';
            }
            const char *state;
            if (job->state == JOB_RUNNIG) {
                state = "Running";
            } else {
                state = "Stopped";
            }
            printf("[%d]%c  %s\t\t%s\n", job->id, marker, job->state, job->command);
        }
    }
    return 0;
}

int builtin_fg(char **args) {
    if (args[1] == NULL) {
        return 0;
    }
    Job *job = find_job_by_spec(args[1]);
    if (job == NULL) {
        perror("fg: no such job\n");
        return 1;
    }
    if (job->command) {
        printf("%s\n", job->command);
    }

    job->state = JOB_RUNNIG;
    tcsetpgrp(STDIN_FILENO, job->pgid);

    if (kill(-job->pgid, SIGCONT) < 0) {
        perror("kill Sigcount");
    }

    int status;
    waitpid(-job->pgid, &status, WUNTRACED);

    tcsetpgrp(STDIN_FILENO, getpgrp());

    if (WIFSTOPPED(status)) {
        job->state = JOB_STOPPED;
        printf("\n[%d]+  Stopped\t\t%s\n", job->id, job->command);
    } else {
        job->state = JOB_DONE;
        remove_job_by_id(job->id);
    }
    return 0;
}