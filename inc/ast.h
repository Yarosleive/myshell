#pragma once

typedef enum {
    NODE_LIST,
    NODE_AND_OR,
    NODE_PIPELINE,
    NODE_SIMPLE,
    NODE_ARG,
    NODE_SUBSHELL,
    NODE_REDIR
} NodeType;

typedef struct Expr {
    NodeType type;
    struct Expr *left;
    char *text;
    struct Expr *right;
} Expr;

void free_ast(Expr *node);