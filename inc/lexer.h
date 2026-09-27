#pragma once

#include "token.h"

typedef struct {
    char *input;
    int pos;
    int len;
} Lexer;

Tokenlist tokenize(Lexer *lexer);
Lexer make_lexer(char *input);

Token next_token(Lexer *lexer);
Token read_word(Lexer *lexer);
Token read_operator(Lexer *lexer);

char peek(Lexer *lexer, int offset);
void advance(Lexer *lexer, int count);
int match(Lexer *lexer, char expected);
int at_end(Lexer *lexer);
