#pragma once

#include "ast.h"
#include "token.h"

typedef struct {
    Tokenlist *tokens;
    int pos;
    int has_error;
} Parser;

Token p_peek(Parser *parser, int offset);
Token p_previous(Parser *parser);
int p_check(Parser *parser, TokenType type);
int p_match(Parser *parser, TokenType type);
Token *p_advance(Parser *parser);
void p_expect(Parser *parser, TokenType type);

Parser make_parser(Tokenlist *tokens);
ExprNode *parse(Parser *parser);
ExprNode  *parse_list(Parser *parser);
ExprNode  *parse_and_or(Parser *parser);
ExprNode  *parse_pipeline(Parser *parser);
ExprNode  *parse_command(Parser *parser);
ExprNode  *parse_subshell(Parser *parser);
ExprNode  *parse_simple(Parser *parser);
ExprNode  *parse_redirection(Parser *parser);
