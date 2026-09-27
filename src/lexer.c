#include "token.h"
#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

int is_operator_char(char c) {
    return c == '|' || c == '&' || c == ';' ||
        c == '>' || c == '<' || c == '(' || c == ')';
}

int is_word_start(char c) {
    return c != '\0' && !isspace(c) && !is_operator_char(c);
}

Lexer make_lexer(const char *input) {
    Lexer lexer;
    lexer.pos = 0;
    lexer.len = strlen(input);
    lexer.input = malloc(sizeof(char) * (lexer.len + 1));
    strcpy(input, lexer.input);
    return lexer;
}

Tokenlist tokenize(Lexer *lexer) {
    Tokenlist tokens;
    tokens.cap = 4;
    tokens.len = 0;
    tokens.data = malloc(sizeof(Token) * tokens.cap);

    while (1) {
        Token token = next_token(lexer);
        if (tokens.len >= tokens.cap - 1) {
            tokens.cap *= 2;
            tokens.data = realloc(tokens.data, tokens.cap);
        }
        tokens.data[(tokens.len)++] = token;
        if (token.type == TOKEN_EOF) {
            break;
        }
    }
    return tokens;
}

Token next_token(Lexer *lexer) {
    while (!at_end(lexer) && isspace(peek(lexer, 0))) {
        advance(lexer, 1);
    }

    Token token;
    token.lenth = 0;
    token.value = NULL;

    if (at_end(lexer)) {
        token.type = TOKEN_EOF;
        return token;
    }

    if (is_word_start(peek(lexer, 0))) {
        return read_word(lexer);
    }

    return read_operator(lexer);
}

Token read_word(Lexer *lexer) {
    int start = lexer->pos;
    while (!at_end(lexer) && is_word_start(peek(lexer, 0))) {
        advance(lexer, 1);
    }

    Token token;
    token.type = TOKEN_WORD;
    token.lenth = lexer->pos - start;
    token.value = strndup(lexer->input + start, token.lenth);
    return token;
}

Token read_operator(Lexer *lexer) {
    int start = lexer->pos;
    while (!at_end(lexer) && is_operator_char(peek(lexer, 0))) {
        advance(lexer, 1);
    }
    Token token;
    token.lenth = lexer->pos - start;
    token.value = strndup(lexer->input + start, token.lenth);
    if (token.lenth == 2) {
        switch (token.value[0]) {
            case '|':
                token.type = TOKEN_OR_IF;
                break;
            case '&':
                token.type = TOKEN_AND_IF;
                break;
            case '>':
                token.type = TOKEN_GG;
                break;
            default:
                perror("Unknown 2 digit operator");
                break;
        }
    } else if (token.lenth == 1) {
        switch (token.value[0]) {
            case '|':
                token.type = TOKEN_PIPE;
                break;
            case '&':
                token.type = TOKEN_AMPERSAND;
                break;
            case '>':
                token.type = TOKEN_GREATER;
                break;
            case '<':
                token.type = TOKEN_LESS;
                break;
            case ';':
                token.type = TOKEN_SEMICOLON;
                break;
            case '(':
                token.type = TOKEN_LPAREN;
                break;
            case ')':
                token.type = TOKEN_RPAREN;
                break;
            default:
                perror("Unknown 1 digit operator");
                break;
        }
    } else {
        perror("unknown operator");
    }
    return token;
}

char peek(Lexer *lexer, int offset) {
    int index = lexer->pos + offset;
    if (index >= lexer->len) {
        return '\0';
    }
    return lexer->input[index];
}

void advance(Lexer *lexer, int count) {
    lexer->pos += count;
}

int match(Lexer *lexer, char expected) {
    return !at_end(lexer) && peek(lexer, 0) == expected;
}

int at_end(Lexer *lexer) {
    return lexer->pos >= lexer->len;
}
