#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "executor.h"

// Функция print_ast может остаться для отладки, если захотите

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
                i++;
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

    while (1) {
        printf("myshell$ ");
        fflush(stdout);
        input[0] = '\0';

        while (1) {
            if (fgets(line, sizeof(line), stdin) == NULL) {
                printf("\n");
                return 0;
            }

            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
                len--;
            }

            int ends_with_backslash = 0;
            if (len > 0 && line[len - 1] == '\\') {
                ends_with_backslash = 1;
                line[len - 1] = '\0';
            }

            if (strlen(input) > 0) {
                strcat(input, "\n");
            }
            strcat(input, line);

            if (ends_with_backslash || has_unclosed_quotes(input)) {
                printf("> ");
                fflush(stdout);
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

        // Если парсер нашел ошибку, не выполняем
        if (parser.has_error || root == NULL) {
            free_ast(root);
            for (int i = 0; i < tokens.len; i++) {
                if (tokens.data[i].value) {
                    free(tokens.data[i].value);
                }
            }
            free(tokens.data);
            continue;
        }

        // 3. Выполнение AST (foreground режим: bg = 0)
        execute(root, 0);

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