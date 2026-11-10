#include "builtins.h"
#include "jobs.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

Builtin builtins[] = {
    {"cd", builtin_cd},
    {"pwd", builtin_pwd},
    {"exit", builtin_exit},
    {"echo", builtin_echo},
    {"help", builtin_help},
    {"jobs", builtin_jobs},
    {"fg", builtin_fg},
    {"bg", builtin_bg},
    {"kill", builtin_kill},
    {"wait", builtin_wait},
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
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
    }
    if (chdir(path) != 0) {
        fprintf(stderr, "cd\n");
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
    fprintf(stderr, "pwd\n");
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
    puts("| `wait [pid|%n]` | ожидание завершения заданий |");
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
            if (job->state == JOB_RUNNING) {
                state = "Running";
            } else {
                state = "Stopped";
            }
            printf("[%d]%c  %s\t\t%s\n", job->id, marker, state, job->command);
        }
    }
    return 0;
}

int builtin_fg(char **args) {
    Job *job = find_job_by_spec(args[1]);
    if (job == NULL) {
        fprintf(stderr, "no such job\n");
        return 1;
    }

    if (tcsetpgrp(STDIN_FILENO, job->pgid) < 0) {
        fprintf(stderr, "tcsetpgrp\n");
        return 1;
    }

    if (job->state == JOB_STOPPED) {
        if (kill(-job->pgid, SIGCONT) < 0) {
            fprintf(stderr, "kill sigcount\n");
            tcsetpgrp(STDIN_FILENO, getpgrp());
            return 1;
        }
    }

    job->state = JOB_RUNNING;

    int status;
    if (waitpid(-job->pgid, &status, WUNTRACED) < 0) {
        fprintf(stderr, "waitpid\n");
        tcsetpgrp(STDIN_FILENO, getpgrp());
        return 1;
    }

    if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0) {
        fprintf(stderr, "mysh: fg: failed to regain terminal control\n");
    }

    if (WIFSTOPPED(status)) {
        job->state = JOB_STOPPED;
        printf("\n[%d]+  Stopped\t\t%s\n", job->id, job->command);
    } else {
        job->state = JOB_DONE;
        remove_job_by_id(job->id);
    }

    update_job_pointers();
    return 0;
}

int builtin_bg(char **args) {
    Job *job = find_job_by_spec(args[1]);
    if (job == NULL) {
        fprintf(stderr, "bg: no such job\n");
        return 1;
    }

    if (job->state == JOB_RUNNING) {
        printf("Job is already in bg\n");
        return 1;
    }

    JobState old_state = job->state;
    job->state = JOB_RUNNING;

    if (kill(-job->pgid, SIGCONT) < 0) {
        fprintf(stderr, "kill\n");
        job->state = old_state;
        return 1;
    }

    if (job->command != NULL) {
        printf("[%d]  %s &\n", job->id, job->command);
    } else {
        printf("[%d]  %s &\n", job->id, "");
    }
    update_job_pointers();
    return 0;
}

int parse_signame(char *signame) {
    if (signame[0] >= '0' && signame[0] <= '9') {
        return atoi(signame);
    }
    if (strncmp(signame, "SIG", 3) == 0) {
        signame += sizeof(char) * 3;
    }

    if (strcmp(signame, "TERM") == 0)
        return SIGTERM;
    if (strcmp(signame, "KILL") == 0)
        return SIGKILL;
    if (strcmp(signame, "INT") == 0)
        return SIGINT;
    if (strcmp(signame, "STOP") == 0)
        return SIGSTOP;
    if (strcmp(signame, "CONT") == 0)
        return SIGCONT;
    if (strcmp(signame, "HUP") == 0)
        return SIGHUP;
    if (strcmp(signame, "QUIT") == 0)
        return SIGQUIT;
    if (strcmp(signame, "USR1") == 0)
        return SIGUSR1;
    if (strcmp(signame, "USR2") == 0)
        return SIGUSR2;
    if (strcmp(signame, "ALRM") == 0)
        return SIGALRM;
    if (strcmp(signame, "PIPE") == 0)
        return SIGPIPE;

    return -1;
}

int builtin_kill(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "not enough args in kill: kill [-s sig | -sig] pid | jobspec ...\n");
        return 1;
    }
    
    int sig = SIGTERM;
    int arg_ind = 1;

    if (args[1][0] == '-') {
        if (strcmp(args[1], "-s") == 0) {
            if (args[2] == NULL) {
                fprintf(stderr, "kill: option requires an arg\n");
                return 1;
            }
            sig = parse_signame(args[2]);
            arg_ind = 3;
        } else {
            sig = parse_signame(args[1] + 1);
            arg_ind = 2;
        }

        if (sig < 0) {
            fprintf(stderr, "wrong signal\n");
            return 1;
        }
    }

    if (args[arg_ind] == NULL) {
        fprintf(stderr, "not enough args in kill: kill [-s sig | -sig] pid | jobspec ...\n");
        return 1;
    }

    int status = 0;

    for (int i = arg_ind; args[i] != NULL; ++i) {
        char *target = args[i];

        if (target[0] == '%') {
            Job *job = find_job_by_spec(target);
            if (job == NULL) {
                fprintf(stderr, "no such job\n");
                status = 1;
                continue;
            }

            if (kill(-job->pgid, sig) < 0) {
                fprintf(stderr, "kill\n");
                status = 1;
            }
        } else {
            pid_t pid = (pid_t)atoi(target);
            if (pid == 0 && target[0] != '0') {
                fprintf(stderr, "args should be process or jobs\n");
                status = 1;
                continue;;
            }

            if (kill(pid, sig) < 0) {
                fprintf(stderr, "kill\n");
                status = 1;
            }
        }
    }

    return status;
}

int builtin_wait(char **args) {
    int last_status = 0;

    if (args[1] == NULL) {
        while (has_active_jobs()) {
            int status;
            pid_t pid = waitpid(-1, &status, WUNTRACED);
            if (pid < 0) {
                break;
            }
        }
        update_jobs();
        return 0;
    }

    char *target = args[1];
    Job *job = NULL;

    if (target[0] == '%' || find_job_by_spec(target) != NULL) {
        job = find_job_by_spec(target);
        if (job == NULL) {
            fprintf(stderr, "no such job\n");
            return 1;
        }
        pid_t pgid = job->pgid;
        int status = 0;

        while (find_job_by_id(job->id) != NULL && job->state != JOB_DONE) {
            pid_t p = waitpid(-pgid, &status, WUNTRACED);
            if (p < 0) {
                break;
            }

            update_jobs();

            if (WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                last_status = 128 + WTERMSIG(status);
            }
        }
    } else {
        pid_t pid = (pid_t)atoi(target);
        if (pid <= 0) {
            fprintf(stderr, "pid or job spec not found\n");
            return 127;
        }

        int status;
        if (waitpid(pid, &status, WUNTRACED) < 0) {
            fprintf(stderr, "wait\n");
            return 127;
        }

        if (WIFEXITED(status)) {
            last_status = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            last_status = 128 + WTERMSIG(status);
        }
    }
    return last_status;
}