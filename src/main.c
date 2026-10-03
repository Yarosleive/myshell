#include <stdio.h>
#include <stdlib.h>
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"

void print_ast(Expr *node, int depth) {
    if (!node) return;
    
    for (int i = 0; i < depth; i++) printf("  ");

    const char *node_type_names[] = {
        "LIST", "AND_OR", "PIPELINE", "SIMPLE", "ARG", "SUBSHELL", "REDIR"
    };

    printf("[%s] text: %s\n", 
        node_type_names[node->type], 
        node->text ? node->text : "NULL");

    if (node->left) {
        for (int i = 0; i < depth + 1; i++) printf("  ");
        printf("left ->\n");
        print_ast(node->left, depth + 2);
    }
    
    if (node->right) {
        for (int i = 0; i < depth + 1; i++) printf("  ");
        printf("right ->\n");
        print_ast(node->right, depth + 2);
    }
}

int main(void) {
    char test_input[256];
    
    printf("Enter command: ");
    if (!fgets(test_input, 256, stdin)) {
        return 0;
    }
    printf("\nInput string: %s\n", test_input);

    Lexer lexer = make_lexer(test_input);
    Tokenlist tokens = tokenize(&lexer);

    printf("--- Tokens ---\n");
    for (int i = 0; i < tokens.len; i++) {
        Token t = tokens.data[i];
        printf("[%d] Type: %-15s | Value: %s\n", 
            i, 
            token_type_name(t.type), 
            t.value ? t.value : "NULL");
    }
    printf("\n");

    Parser parser = make_parser(&tokens);
    Expr *root = parse(&parser);

    printf("--- AST Structure ---\n");
    print_ast(root, 0);

    free_ast(root);
    free_tokenlist(&tokens);
    free((void*)lexer.input);

    return 0;
}
