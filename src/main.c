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

void print_ast(ExprNode *node, int indent) {
    if (node == NULL) return;

    for (int i = 0; i < indent; i++) printf("  ");

    switch (node->type) {
        case NODE_LIST:
            printf("NODE_LIST (op: %d)\n", node->binary.op);
            print_ast(node->binary.left, indent + 1);
            print_ast(node->binary.right, indent + 1);
            break;

        case NODE_AND_OR:
            printf("NODE_AND_OR (op: %d)\n", node->binary.op);
            print_ast(node->binary.left, indent + 1);
            print_ast(node->binary.right, indent + 1);
            break;

        case NODE_PIPELINE:
            printf("NODE_PIPELINE\n");
            print_ast(node->binary.left, indent + 1);
            print_ast(node->binary.right, indent + 1);
            break;

        case NODE_SIMPLE:
            printf("NODE_SIMPLE (argc: %d)\n", node->simple.argc);
            for (int i = 0; i < node->simple.argc; i++) {
                for (int j = 0; j <= indent; j++) printf("  ");
                printf("arg[%d]: \"%s\"\n", i, node->simple.args[i]);
            }
            if (node->simple.redirs) {
                for (int j = 0; j <= indent; j++) printf("  ");
                printf("Redirections:\n");
                print_ast(node->simple.redirs, indent + 2);
            }
            break;

        case NODE_REDIR:
            printf("NODE_REDIR (op: %d, file: \"%s\")\n", node->redir.op, node->redir.fname ? node->redir.fname : "NULL");
            if (node->redir.next) {
                print_ast(node->redir.next, indent);
            }
            break;

        case NODE_SUBSHELL:
            printf("NODE_SUBSHELL\n");
            for (int j = 0; j <= indent; j++) printf("  ");
            printf("Inner AST:\n");
            print_ast(node->subshell.inner, indent + 2);
            if (node->subshell.redirs) {
                for (int j = 0; j <= indent; j++) printf("  ");
                printf("Redirections:\n");
                print_ast(node->subshell.redirs, indent + 2);
            }
            break;

        case NODE_ARG:
            printf("NODE_ARG\n");
            break;

        default:
            printf("UNKNOWN_NODE\n");
            break;
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

int run_command_line(char *line, int dump_tokens, int dump_ast) {
    char *cmd_line_copy = strdup(line);
    int len = strlen(cmd_line_copy);
    if (len > 0 && cmd_line_copy[len - 1] == '\n') {
        cmd_line_copy[len - 1] = '\0';
    }
    if (cmd_line_copy[0] == '\0' || cmd_line_copy[0] == '#') {
        free(cmd_line_copy);
        return 0;
    }

    Lexer lexer = make_lexer(line);
    Tokenlist tokens = tokenize(&lexer);

    if (lexer.has_error || tokens.data == NULL) {
        free(cmd_line_copy);
        return 2;
    }
    
    if (dump_tokens) {
        free(cmd_line_copy);
        return 0;
    }

    Parser parser = make_parser(&tokens);
    ExprNode *ast = parse(&parser);

    if (parser.has_error || ast == NULL) {
        if (tokens.data) {
            free(tokens.data);
        }
        free(cmd_line_copy);
        return 2;
    }

    if (dump_ast) {
        print_ast(ast, 0);
        free_ast(ast);
        free(cmd_line_copy);
        return 0;
    }

    int status = execute(ast, 0, cmd_line_copy);

    free_ast(ast);
    free(cmd_line_copy);
    return status;
}


int main(int argc, char *argv[]) {
    init_shell();

    int dump_tokens = 0;
    int dump_ast = 0;
    char *cmd_to_run = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dump-tokens") == 0) {
            dump_tokens = 1;
        } else if (strcmp(argv[i], "--dump-ast") == 0) {
            dump_ast = 1;
        } else if (strcmp(argv[i], "-c") == 0) {
            if (i + 1 < argc) {
                cmd_to_run = argv[i + 1];
                i++;
            } else {
                fprintf(stderr, "mysh: option '-c' requires an argument\n");
                return 1;
            }
        }
    }

    if (cmd_to_run != NULL) {
        is_interactive = 0;
        return run_command_line(cmd_to_run, dump_tokens, dump_ast);
    }

    if (dump_tokens || dump_ast) {
        is_interactive = 0;
        char full_line[MAX_LINE * 4] = "";
        char buf[MAX_LINE];
        while (fgets(buf, sizeof(buf), stdin) != NULL) {
            strcat(full_line, buf);
        }
        return run_command_line(full_line, dump_tokens, dump_ast);
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

        run_command_line(full_line, 0, 0);
    }
    return 0;
}
