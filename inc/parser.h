#pragma once

#include "ast.h"
#include "token.h"

typedef struct {
    Tokenlist *tokens;
    int pos;
} Parser;

Token p_peek(Parser *parser, int offset);
Token p_previous(Parser *parser);
int p_check(Parser *parser, TokenType type);
int p_match(Parser *parser, TokenType type);
Token *p_advance(Parser *parser);
void p_expect(Parser *parser, TokenType type);

Parser make_parser(Tokenlist *tokens);
Expr *parse(Parser *parser);
Expr *parse_list(Parser *parser);
Expr *parse_and_or(Parser *parser);
Expr *parse_pipeline(Parser *parser);
Expr *parse_command(Parser *parser);
Expr *parse_subshell(Parser *parser);
Expr *parse_simple(Parser *parser);
Expr *parse_redirection(Parser *parser);
