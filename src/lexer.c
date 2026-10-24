#include "token.h"
#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

/*Идея такая: в tokenize с помощью next_token читаем по токену
Токены бывают 2-х типов: слова и операторы, под каждый тип своя функция.
Лексер слизан с лексера из интерпретатора матфорфул с поправкой
на ограниченность С относительно С++
Лексер у нас хранит строку, текущую позицию и длину строки*/

int is_operator_char(char c) {
    return c == '|' || c == '&' || c == ';' ||
        c == '>' || c == '<' || c == '(' || c == ')';
}

int is_word_start(char c) {
    return c != '\0' && !isspace(c) && !is_operator_char(c);
}

// аналог конструктора
Lexer make_lexer(const char *input) {
    Lexer lexer;
    lexer.pos = 0;
    lexer.len = strlen(input);
    lexer.input = input;
    return lexer;
}

// основная функция создания токенов
Tokenlist tokenize(Lexer *lexer) {
    Tokenlist tokens;
    tokens.cap = 4;
    tokens.len = 0;
    tokens.data = malloc(sizeof(Token) * tokens.cap);

    while (1) {
        // обрабатываем по слову, обновляем ёмкость, коль надо
        Token token = next_token(lexer);
        if (tokens.len >= tokens.cap) {
            tokens.cap *= 2;
            tokens.data = realloc(tokens.data, sizeof(Token) * tokens.cap);
        }
        tokens.data[(tokens.len)++] = token;
        if (token.type == TOKEN_EOF) {
            break;
        }
    }
    return tokens;
}

/*Определяем тип токена: слово или оператор с помощью первого символа
Если строка, идем в read_word, иначе в read_operator*/
Token next_token(Lexer *lexer) {
    while (!at_end(lexer) && isspace(peek(lexer, 0))) {
        advance(lexer, 1);
    }

    Token token;
    token.length = 0;
    token.value = NULL;

    if (at_end(lexer) || peek(lexer, 0) == '#') {
        token.type = TOKEN_EOF;
        return token;
    }

    if (is_word_start(peek(lexer, 0))) {
        return read_word (lexer);
    }

    return read_operator(lexer);
}

char *read_quote(Lexer *lexer) {
    char quote = peek(lexer, 0);
        advance(lexer, 1);
    int start = lexer->pos;
    while (peek(lexer, 0) != quote && !at_end(lexer) ) {
        if (quote == '"' && peek(lexer, 0) == '\\' &&
            (peek(lexer, 1) == '\\' || peek(lexer, 1) == quote)) {
                advance(lexer, 1);
        }
        advance(lexer, 1);
    }
    if (at_end(lexer)) {
        perror("not closed quote");
        exit(1);
    }
    advance(lexer, 1);
    
    char *word = malloc(sizeof(char) * (lexer->pos - start));
    int j = 0;
    for (int i = start; i < lexer->pos - 1; ++i) {
        if (quote == '"' && lexer->input[i] == '\\' &&
            (lexer->input[i + 1] == '\\' || lexer->input[i + 1] == quote)) {
                ++i;
                word[j++] = lexer->input[i];
                continue;
        }
        word[j++] = lexer->input[i];
        
    }
    word[j] = '\0';
    return word;
}

/*Слово может начинаться с кавычки, обрабатываем этот случай
Кавычки в токен не записываются
Кавычки "" и '' отличаются друг от друга*/
Token read_word(Lexer *lexer) {
    Token token;
    token.type = TOKEN_WORD;

    int cap = 32;
    int len = 0;
    char *value = malloc(sizeof(char) * cap);

    while (!at_end(lexer)) {
        char c = peek(lexer, 0);
        if (isspace(c) || is_operator_char(c)) {
            break;
        }
        char *part = NULL;
        int part_len = 1;
        if (c == '\'' || c == '"') {
            part = read_quote(lexer);
            part_len = strlen(part);
        }
        if (c == '\\') {
            advance(lexer, 1);
            if (peek(lexer, 0) == '\n') {
                advance(lexer, 1);
            }
            c = peek(lexer, 0);
        }
        if (len + part_len >= cap) {
            cap *= 2;
            value = realloc(value, sizeof(char) * cap);
        }

        if (part != NULL) {
            strcpy(value + len, part);
            len += part_len;
        } else {
            value[len++] = c;
            advance(lexer, 1);
        }
        free(part);
    }
    value[len] = '\0';
    token.value = value;
    token.length = len;

    return token;
}

/*Читаем оператор. тут даже объяснять нечего*/
Token read_operator(Lexer *lexer) {
    int start = lexer->pos;
    while (!at_end(lexer) && is_operator_char(peek(lexer, 0))) {
        advance(lexer, 1);
    }
    Token token;
    token.length = lexer->pos - start;
    token.value = strndup(lexer->input + start, token.length);

    switch (token.value[0]) {
        case '|':
            if (token.length == 1)
                token.type = TOKEN_PIPE;
            else
                token.type = TOKEN_OR_IF;
            break;
        case '&':
            if (token.length == 1)
                token.type = TOKEN_AMPERSAND;
            else
                token.type = TOKEN_AND_IF;
            break;
        case '>':
            if (token.length == 1)
            token.type = TOKEN_GREATER;
            else
                token.type = TOKEN_GG;
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
            perror("Unknown operator");
            exit(1);
    }
    return token;
}

// возвращаем элемент в позиции пос + офсет
char peek(Lexer *lexer, int offset) {
    int index = lexer->pos + offset;
    if (index >= lexer->len) {
        return '\0';
    }
    return lexer->input[index];
}

// двигаем позицию на каунт
void advance(Lexer *lexer, int count) {
    lexer->pos += count;
}

// соответствует ли символ в позиции символу экспектид
int match(Lexer *lexer, char expected) {
    return !at_end(lexer) && peek(lexer, 0) == expected;
}

// конец ма?
int at_end(Lexer *lexer) {
    return lexer->pos >= lexer->len;
}
