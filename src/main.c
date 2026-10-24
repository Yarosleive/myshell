#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"

void print_ast(ExprNode *node, int depth) {
    if (!node) return;
    
    for (int i = 0; i < depth; i++) printf("  ");

    switch (node->type) {
        case NODE_LIST:
            printf("LIST Op: %d\n", node->binary.op);
            print_ast(node->binary.left, depth + 1);
            print_ast(node->binary.right, depth + 1);
            break;
        case NODE_AND_OR:
            printf("AND_OR Op: %d\n", node->binary.op);
            print_ast(node->binary.left, depth + 1);
            print_ast(node->binary.right, depth + 1);
            break;
        case NODE_PIPELINE:
            printf("PIPELINE\n");
            print_ast(node->binary.left, depth + 1);
            print_ast(node->binary.right, depth + 1);
            break;
        case NODE_SIMPLE:
            printf("SIMPLE CMD [Args: ");
            for (int i = 0; i < node->simple.argc; i++) {
                printf("'%s' ", node->simple.args[i]);
            }
            printf("]\n");
            if (node->simple.redirs) {
                print_ast(node->simple.redirs, depth + 1);
            }
            break;
        case NODE_REDIR:
            printf("REDIR Op: %d, File: '%s'\n", node->redir.op, node->redir.fname);
            if (node->redir.next) {
                print_ast(node->redir.next, depth);
            }
            break;
        case NODE_SUBSHELL:
            printf("SUBSHELL\n");
            print_ast(node->subshell.inner, depth + 1);
            if (node->subshell.redirs) {
                print_ast(node->subshell.redirs, depth + 1);
            }
            break;
        default:
            printf("UNKNOWN NODE\n");
            break;
    }
}

// Проверяет, есть ли в строке незакрытые одинарные или двойные кавычки
int has_unclosed_quotes(const char *str) {
    int in_single = 0;
    int in_double = 0;
    
    for (int i = 0; str[i] != '\0'; i++) {
        if (in_single) {
            if (str[i] == '\'') {
                in_single = 0;
            }
        } else if (in_double) {
            if (str[i] == '\\' && str[i + 1] != '\0') {
                i++; // Пропускаем экранированный символ внутри двойных кавычек
            } else if (str[i] == '"') {
                in_double = 0;
            }
        } else {
            if (str[i] == '\'') {
                in_single = 1;
            } else if (str[i] == '"') {
                in_double = 1;
            }
        }
    }
    return in_single || in_double;
}

int main(int argc, char **argv) {
    char input[8192] = "";
    char line[1024];

    printf("Interactive Shell Parser Test. Type 'exit' to quit.\n\n");

    while (1) {
        printf("my_shell> ");
        input[0] = '\0';

        while (1) {
            if (fgets(line, sizeof(line), stdin) == NULL) {
                printf("\nExiting...\n");
                return 0;
            }

            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
                len--;
            }

            // Проверяем бэкслеш в самом конце строки (вне кавычек/продолжение)
            int ends_with_backslash = 0;
            if (len > 0 && line[len - 1] == '\\') {
                // Убедимся, что бэкслеш не сам экранирован
                ends_with_backslash = 1;
                line[len - 1] = '\0'; // Убираем бэкслеш
            }

            // Дописываем текущую строку к общему буферу
            // Если это не первая итерация многострочного ввода, возвращаем перевод строки,
            // который съел fgets, или пробел (в зависимости от логики). 
            // Для кавычек перевод строки внутри сохраняется как часть строки.
            if (strlen(input) > 0) {
                strcat(input, "\n");
            }
            strcat(input, line);

            // Условие продолжения ввода:
            // 1. Строка заканчивалась на неэкранированный '\'
            // 2. Либо есть незакрытые кавычки
            if (ends_with_backslash || has_unclosed_quotes(input)) {
                printf("> "); 
                continue;
            } else {
                break;
            }
        }

        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            break;
        }
        if (strlen(input) == 0) {
            continue;
        }

        // 1. Лексический анализ
        Lexer lexer = make_lexer(input);
        Tokenlist tokens = tokenize(&lexer);

        // 2. Синтаксический анализ
        Parser parser = make_parser(&tokens);
        ExprNode *root = parse(&parser);

        // 3. Вывод AST
        printf("\n--- AST Structure ---\n");
        print_ast(root, 0);
        printf("---------------------\n\n");

        // 4. Очистка памяти
        free_ast(root);
        for (int i = 0; i < tokens.len; i++) {
            if (tokens.data[i].value) {
                free(tokens.data[i].value);
            }
        }
        free(tokens.data);
    }

    return 0;
}