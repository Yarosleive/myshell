#pragma once

#include <stdlib.h>
#include <string.h>

typedef enum {
    TOKEN_WORD, // word

    // Operators
    TOKEN_PIPE, // |
    TOKEN_AMPERSAND, // &
    TOKEN_SEMICOLON, // ;
    TOKEN_AND_IF, // &&
    TOKEN_OR_IF, // ||
    TOKEN_LPAREN, // (
    TOKEN_RPAREN, // )
    TOKEN_LESS, // <
    TOKEN_GREATER, // >
    TOKEN_GG, // >>

    TOKEN_EOF // end of file
} TokenType;

typedef struct {
    TokenType type;
    char *value;
    int length; // длина лексемы
} Token;

typedef struct {
    Token *data;
    int len;
    int cap;
} Tokenlist;

void free_token(Token *token);
void free_tokenlist(Tokenlist *list);
char *token_type_name(TokenType type);