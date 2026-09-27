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
    int lenth; // длина лексемы
} Token;

typedef struct {
    Token *data;
    int len;
    int cap;
} Tokenlist;

void free_token(Token *token) {
    if (token && token->value) {
        free(token->value);
        token->value = NULL;
    }
}

void free_tokenlist(Tokenlist *list) {
    for (int i = 0; i < list; ++i) {
        free_token(&list->data[i]);
    }
    free(list->data);
}

char *token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_WORD:
            return "word";
        case TOKEN_PIPE:
            return "|";
        case TOKEN_AMPERSAND:
            return "&";
        case TOKEN_SEMICOLON:
            return ";";
        case TOKEN_AND_IF:
            return "&&";
        case TOKEN_OR_IF:
            return "||";
        case TOKEN_LPAREN:
            return "(";
        case TOKEN_RPAREN:
            return ")";
        case TOKEN_LESS:
            return "<";
        case TOKEN_GREATER:
            return ">";
        case TOKEN_GG:
            return ">>";
        case TOKEN_EOF:
            "eof";
        default:
            return "unknown token";
    }
}
