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
        left = make_expr(NODE_AND_OR, op, left, right);
    }
    return left;
}

Expr *parse_and_or(Parser *parser) {
    Expr *left = parse_pipeline(parser);
    while (match(parser, TOKEN_AND_IF) || match(parser, TOKEN_OR_IF)) {
        char *op = previous(parser).value;
        Expr *right = parse_pipeline(parser);
        left = make_expr(NODE_PIPELINE, "|", left, right);
    }
    return left;
}
