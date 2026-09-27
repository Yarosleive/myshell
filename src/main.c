#include <stdio.h>
#include <stdlib.h>
#include "token.h"
#include "lexer.h"

int main(void) {
    // Тестовая строка со словами, флагами и операторами
    char test_input[256];
    
    fgets(test_input, 256, stdin);
    printf("Input string: %s\n\n", test_input);

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

    // Не забываем освободить выделенную память!
    free((void*)lexer.input); // освобождаем строку внутри лексера
    free_tokenlist(&tokens);  // освобождаем токены и сам массив

    return 0;
}
