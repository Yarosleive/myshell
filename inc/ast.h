#pragma once

// типы узлов дерева
typedef enum {
    NODE_LIST,
    NODE_AND_OR,
    NODE_PIPELINE,
    NODE_SIMPLE,
    NODE_ARG,
    NODE_SUBSHELL,
    NODE_REDIR
} NodeType;

typedef enum {
    OP_OR_IF,
    OP_AND_IF,
    OP_SEMICOLON,
    OP_PIPE,
    OP_AMPERSAND
} OpType;

typedef enum {
    REDIR_LESS,
    REDIR_GREATER,
    REDIR_GG
} RedirOp;

typedef struct ExprNode {
    NodeType type;
    union {
        struct {
            struct ExprNode *left;
            struct ExprNode *right;
            OpType op;
        } binary;

        struct {
            struct ExprNode *redirs;
            char **args;
            int argc;
        } simple;

        struct {
            struct ExprNode *next;
            char *fname;
            RedirOp op;
        } redir;

        struct {
            struct ExprNode *inner;
            struct ExprNode *redirs;
        } subshell;

        struct {
            char *value;
        } word;
    };
} ExprNode;

void free_ast(ExprNode  *node);
