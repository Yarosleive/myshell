#pragma once

#include "ast.h"
#include "token.h"

typedef struct {
    Tokenlist *tokens;
    int pos;
} Parser;

Token peek(Parser *parser, int offset);
Token previous(Parser *parser);
int check(Parser *parser, TokenType type);
int match(Parser *parser, TokenType type);
Token *advance(Parser *parser);
void expect(Parser *parser, TokenType type);

Parser make_parser(Tokenlist *tokens);
Expr *parse(Parser *parser);
Expr *parse_list(Parser *parser);
Expr *parse_and_or(Parser *parser);
Expr *parse_pipeline(Parser *parser);
Expr *parse_command(Parser *parser);
Expr *parse_subshell(Parser *parser);
Expr *parse_simple(Parser *parser);
Expr *parse_redirection(Parser *parser);
