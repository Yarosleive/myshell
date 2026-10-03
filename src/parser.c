#include "parser.h"
#include "ast.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

Parser make_parser(Tokenlist *tokens) {
    Parser parser;
    parser.tokens = tokens;
    parser.pos = 0;
    return parser;
}

Expr *make_expr(NodeType type, char *text, Expr *left, Expr *right) {
    Expr *expr = malloc(sizeof(Expr));
    if (!expr) {
        perror("malloc failed for Expr");
        exit(1);
    }
    expr->type = type;
    if (text != NULL) {
        expr->text = strdup(text);
    } else {
        expr->text = NULL;
    }
    
    expr->left = left;
    expr->right = right;
    return expr;
}

Expr *parse(Parser *parser) {
    Expr *root = parse_list(parser);
    p_expect(parser, TOKEN_EOF);
    return root;
}

Expr *parse_list(Parser *parser) {
    Expr *left = parse_and_or(parser);
    while (p_match(parser, TOKEN_SEMICOLON) || p_match(parser, TOKEN_AMPERSAND)) {
        char *op = p_previous(parser).value;
        Expr *right = NULL;
        if (!p_check(parser, TOKEN_EOF) && !p_check(parser, TOKEN_RPAREN)) {
            right = parse_and_or(parser);
        }
        left = make_expr(NODE_LIST, op, left, right);
    }
    return left;
}

Expr *parse_and_or(Parser *parser) {
    Expr *left = parse_pipeline(parser);
    while (p_match(parser, TOKEN_AND_IF) || p_match(parser, TOKEN_OR_IF)) {
        char *op = p_previous(parser).value;
        Expr *right = parse_pipeline(parser);
        left = make_expr(NODE_AND_OR, op, left, right);
    }
    return left;
}

Expr *parse_pipeline(Parser *parser) {
    Expr *left = parse_command(parser);
    while (p_match(parser, TOKEN_PIPE)) {
        char *op = p_previous(parser).value;
        Expr *right = parse_command(parser);
        left = make_expr(NODE_PIPELINE, op, left, right);
    }
    return left;
}

Expr *parse_command(Parser *parser) {
    if (p_match(parser, TOKEN_LPAREN)) {
        return parse_subshell(parser);
    }
    return parse_simple(parser);
}

Expr *parse_subshell(Parser *parser) {
    Expr *list= parse_list(parser);
    p_expect(parser, TOKEN_RPAREN);
    Expr *subshell = make_expr(NODE_SUBSHELL, NULL, list, NULL);
    Expr *last_redir = NULL;
    while (p_check(parser, TOKEN_LESS) || p_check(parser, TOKEN_GREATER) || p_check(parser, TOKEN_GG)) {
        Expr *redir = parse_redirection(parser);
        if (subshell->right == NULL) {
            subshell->right = redir;
        } else {
            last_redir->right = redir; // цепляем цепочку перенаправлений
        }
        last_redir = redir;
    }
    return list;
}

Expr *parse_redirection(Parser *parser) {
    Token op = p_peek(parser, 0);
    if (p_match(parser, TOKEN_LESS) || p_match(parser, TOKEN_GREATER) || p_match(parser, TOKEN_GG)) {
        if (!p_check(parser, TOKEN_WORD)) {
            perror("expected filename after redirection");
            exit(1);
        }
        Token fname = *p_advance(parser);
        Expr *fname_node = make_expr(NODE_ARG, fname.value, NULL, NULL);
        return make_expr(NODE_REDIR, op.value, fname_node, NULL);
    }
    return NULL;
}

Expr *parse_simple(Parser *parser) {
    Expr *first_redir = NULL;
    Expr *last_redir = NULL;
    while (p_check(parser, TOKEN_LESS) || p_check(parser, TOKEN_GREATER) || p_check(parser, TOKEN_GG)) {
        Expr *redir = parse_redirection(parser);
        if (first_redir == NULL) {
            first_redir = redir;
        } else {
            last_redir->right = redir;
        }
        last_redir = redir;
    }

    if (!p_check(parser, TOKEN_WORD)) {
        perror("expected command name");
        exit(1);
    }

    Token command = *p_advance(parser);
    Expr *simple = make_expr(NODE_SIMPLE, command.value, NULL, NULL);
    simple->right = first_redir;

    if (last_redir == NULL && simple->right != NULL) {
        last_redir = simple->right;
        while (last_redir->right != NULL) {
            last_redir = last_redir->right;
        }
    }

    while (p_check(parser, TOKEN_WORD) || p_check(parser, TOKEN_LESS) ||
            p_check(parser, TOKEN_GREATER) || p_check(parser, TOKEN_GG)) {
        if (p_check(parser, TOKEN_LESS) || p_check(parser, TOKEN_GREATER) || p_check(parser, TOKEN_GG)) {
            Expr *redir = parse_redirection(parser);
            if (simple->right == NULL) {
                simple->right = redir;
            } else {
                last_redir->right = redir;
            }
            last_redir = redir;
        } else {
            Token arg = *p_advance(parser);
            Expr *arg_node = make_expr(NODE_ARG, arg.value, NULL, NULL);
            if (simple->left == NULL) {
                simple->left = arg_node;
            } else {
                Expr *cur = simple->left;
                while (cur->right != NULL) {
                    cur = cur->right;
                }
                cur->right = arg_node;
            }
        }
    }
    return simple;
}

Token p_peek(Parser *parser, int offset) {
    if (parser->pos + offset >= parser->tokens->len) {
        perror("peek");
        exit(1);
    }
    return parser->tokens->data[parser->pos + offset];
}

Token p_previous(Parser *parser) {
    if (parser->pos - 1 < 0) {
        perror("previous function");
        exit(1);
    }
    return parser->tokens->data[parser->pos - 1];
}

int p_check(Parser *parser, TokenType type) {
    return p_peek(parser, 0).type == type;
}

int p_match(Parser *parser, TokenType type) {
    if (!p_check(parser, type)) {
        return 0;
    }
    p_advance(parser);
    return 1;
}

Token *p_advance(Parser *parser) {
    if (parser->pos < parser->tokens->len) {
        (parser->pos)++;
    }
    Token *token = &(parser->tokens->data[parser->pos - 1]);
    return token;
}

void p_expect(Parser *parser, TokenType type) {
    if (!p_match(parser, type)) {
        perror("Unexpected token");
        exit(1);
    }
}

void free_ast(Expr *node) {
    if (!node) return;
    free_ast(node->left);
    free_ast(node->right);
    if (node->text) {
        free((void*)node->text);
    }
    free(node);
}
