#include "token.h"
#include <stdlib.h>

void free_token(Token *token) {
    if (token && token->value) {
        free(token->value);
        token->value = NULL;
    }
}

void free_tokenlist(Tokenlist *list) {
    for (int i = 0; i < list->len; ++i) {
        free_token(&list->data[i]);
    }
    free(list->data);
}

char *token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_WORD:
            return "word";
        case TOKEN_PIPE:
            return "| - token pipe";
        case TOKEN_AMPERSAND:
            return "& - token ampersand";
        case TOKEN_SEMICOLON:
            return "; - token semicolon";
        case TOKEN_AND_IF:
            return "&& - token and if";
        case TOKEN_OR_IF:
            return "|| - token or if";
        case TOKEN_LPAREN:
            return "( - token lparen";
        case TOKEN_RPAREN:
            return ") - token rparen";
        case TOKEN_LESS:
            return "< - token less";
        case TOKEN_GREATER:
            return "> - token greater";
        case TOKEN_GG:
            return ">> - token gg";
        case TOKEN_EOF:
            return "eof token";
        default:
            return "unknown token";
    }
}
