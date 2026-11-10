#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "lexer.h"
#include "parser.h"
#include "executor.h"
#include "jobs.h"

#define MAX_LINE 2048

int is_interactive = 0;

void init_shell() {
    is_interactive = isatty(STDIN_FILENO);

    if (is_interactive) {
        pid_t shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0) {
            perror("mysh: couldn't put shell in its own process group");
        }
        
        tcsetpgrp(STDIN_FILENO, shell_pgid);

        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
    }
}

int continue_line(char *line) {
    int len = strlen(line);
    if (len == 0) {
        return 0;
    }
    int idx = len - 1;
    if (line[idx] == '\n') {
        --idx;
    }
    if (idx < 0) {
        return 0;
    }

    if (line[idx] == '\\') {
        int backslash_counter = 0;
        while (idx >= 0 && line[idx] == '\\') {
            ++backslash_counter;
            --idx;
        }
        return (backslash_counter % 2) != 0;
    }

    return 0;
}

int run_command_line(char *line) {
    char *cmd_line_copy = strdup(line);
    int len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
        line[len - 1] = '\0';
    }
    if (line[0] == '\0' || line[0] == '#') {
        return 0;
    }

    Lexer lexer = make_lexer(line);
    Tokenlist tokens = tokenize(&lexer);
    Parser parser = make_parser(&tokens);
    ExprNode *ast = parse(&parser);

    if (ast == NULL) {
        return 1;
    }

    int status = execute(ast, 0, cmd_line_copy);

    free(cmd_line_copy);
    return status;
}

int main(int argc, char *argv[]) {
    init_shell();
    
    if (argc == 3 && strcmp(argv[1], "-c") == 0) {
        return run_command_line(argv[2]);
    }

    char buf[MAX_LINE];
    char full_line[MAX_LINE * 4];
    full_line[0] = '\0';

    while (1) {
        if (is_interactive) {
            printf("mysh> ");
            fflush(stdout);
        }
        
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            if (is_interactive) {
                printf("exit\n");
            }
            break;
        }

        strcpy(full_line, buf);

        while (continue_line(full_line)) {
            int flen = strlen(full_line);
            while (flen > 0 && ((full_line[flen - 1] == '\n') || (full_line[flen - 1] == '\\'))) {
                full_line[flen - 1] = '\0';
                --flen;
            }
            if (is_interactive) {
                printf("> ");
                fflush(stdout);
            }

            char next_buf[MAX_LINE];
            if (fgets(next_buf, sizeof(next_buf), stdin) == NULL) {
                break;
            }
            strcat(full_line, next_buf);
        }

        run_command_line(full_line);
    }
    return 0;
}