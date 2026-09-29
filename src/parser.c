#include "parser.h"
#include "ast.h"
#include <string.h>

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
    }
    expr->type = type;
    expr->text = strdup(text);
    expr->left = left;
    expr->right = right;
    return expr;
}

Expr *parse(Parser *parser) {
    Expr *root = parse_list(parser);
    expect(parser, TOKEN_EOF);
    return root;
}

Expr *parse_list(Parser *parser) {
    Expr *left = parse_and_or(parser);
    while (match(parser, TOKEN_SEMICOLON) || match(parser, TOKEN_AMPERSAND)) {
        char *op = previous(parser).value;
        Expr *right = NULL;
        if (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_RPAREN)) {
            right = parse_and_or(parser);
        }
        left = make_expr(NODE_LIST, op, left, right);
    }
    return left;
}

Expr *parse_and_or(Parser *parser) {
    Expr *left = parse_pipeline(parser);
    while (match(parser, TOKEN_AND_IF) || match(parser, TOKEN_OR_IF)) {
        char *op = previous(parser).value;
        Expr *right = parse_pipeline(parser);
        left = make_expr(NODE_AND_OR, op, left, right);
    }
    return left;
}

Expr *parse_pipeline(Parser *parser) {
    Expr *left = parse_command(parser);
    while (match(parser, TOKEN_PIPE)) {
        char *op = previous(parser).value;
        Expr *right = parse_command(parser);
        left = make_expr(NODE_PIPELINE, op, left, right);
    }
    return left;
}

Expr *parse_command(Parser *parser) {
    if (match(parser, TOKEN_LPAREN)) {
        return parse_subshell(parser);
    }
    return parse_simple(parser);
}

Expr *parse_subshell(Parser *parser) {
    Expr *list= parse_list(parser);
    expect(parser, TOKEN_RPAREN);
    Expr *subshell = make(NODE_SUBSHELL, NULL, list, NULL);
    Expr *last_redir = NULL;
    while (check(parser, TOKEN_LESS) || check(parser, TOKEN_GREATER) || check(parser, TOKEN_GG)) {
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
    Token op = peek(parser, 0);
    if (match(parser, TOKEN_LESS) || match(parser, TOKEN_GREATER) || match(parser, TOKEN_GG)) {
        if (!check(parser, TOKEN_WORD)) {
            perror("expected filename after redirection");
        }
        Token fname = *advance(parser);
        Expr *fname_node = make_expr(NODE_ARG, fname.value, NULL, NULL);
        return make_expr(NODE_REDIR, op.value, fname_node, NULL);
    }
    return NULL;
}

Expr *parse_simple(Parser *parser) {
    Expr *first_redir = NULL;
    Expr *last_redir = NULL;
    while (check(parser, TOKEN_LESS) || check(parser, TOKEN_GREATER) || check(parser, TOKEN_GG)) {
        Expr *redir = parse_redirection(parser);
        if (first_redir == NULL) {
            first_redir = redir;
        } else {
            last_redir->right = redir;
        }
        last_redir = redir;
    }

    if (!check(parser, TOKEN_WORD)) {
        perror("expected command name");
    }

    Token command = *advance(parser);
    Expr *simple = make_expr(NODE_SIMPLE, command.value, NULL, NULL);
    simple->right = first_redir;

    if (last_redir == NULL && simple->right != NULL) {
        last_redir = simple->right;
        while (last_redir->right != NULL) {
            last_redir = last_redir->right;
        }
    }

    Expr *last_arg = NULL;
    while (check(parser, TOKEN_WORD) || check(parser, TOKEN_LESS) ||
            check(parser, TOKEN_GREATER) || check(parser, TOKEN_GG)) {
        if (check(parser, TOKEN_LESS) || check(parser, TOKEN_GREATER) || check(parser, TOKEN_GG)) {
            Expr *redir = parse_redirection(parser);
            if (simple->right == NULL) {
                simple->right = redir;
            } else {
                last_redir->right = redir;
            }
            last_redir = redir;
        } else {
            Token arg = *advance(parser);
            Expr *arg_node = make_expr(NODE_ARG, arg.value, NULL, NULL);
            if (simple->left = NULL) {
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
