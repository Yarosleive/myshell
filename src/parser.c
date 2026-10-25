#include "parser.h"
#include "token.h"
#include "ast.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*Идея такая: существуют разные "уровни" строки интерпретатора
Под каждый урвоень написана своя фунция
Формируем дерево по этим уровням*/

// Аналок конструктора 
Parser make_parser(Tokenlist *tokens) {
    Parser parser;
    parser.tokens = tokens;
    parser.pos = 0;
    parser.has_error = 0;
    return parser;
}

// создать узел дерева
ExprNode *make_binary(NodeType type, ExprNode *left, ExprNode *right, OpType op) {
    ExprNode *expr = malloc(sizeof(ExprNode));
    if (expr == NULL) {
        perror("malloc failed for binary");
        exit(1);
    } 

    expr->type = type;
    expr->binary.left = left;
    expr->binary.right = right;
    expr->binary.op = op;
    return expr;
}

ExprNode *make_simple(NodeType type, ExprNode *redirs, char **args, int argc) {
    ExprNode *expr = malloc(sizeof(ExprNode));
    if (expr == NULL) {
        perror("malloc failed for simple");
        exit(1);
    } 

    expr->type = type;
    expr->simple.args = malloc(sizeof(char *) * (argc + 1));
    for (int i = 0; i < argc; ++i) {
        expr->simple.args[i] = strdup(args[i]);
    }
    expr->simple.args[argc] = NULL;
    expr->simple.redirs = redirs;
    expr->simple.argc = argc;
    return expr;
}

ExprNode *make_redir(NodeType type, ExprNode *next, const char *fname, RedirOp op) {
    ExprNode *expr = malloc(sizeof(ExprNode));
    if (expr == NULL) {
        perror("malloc failed for redir");
        exit(1);
    } 

    expr->type = type;
    expr->redir.next = next;
    expr->redir.fname = strdup(fname);
    expr->redir.op = op;
    return expr;
}

ExprNode *make_subshell(NodeType type, ExprNode *inner, ExprNode *redirs) {
    ExprNode *expr = malloc(sizeof(ExprNode));
    if (expr == NULL) {
        perror("malloc failed for subshell");
        exit(1);
    } 

    expr->type = type;
    expr->subshell.inner = inner;
    expr->subshell.redirs = redirs;
    return expr;
}

ExprNode *make_word(NodeType type, char *value) {
    ExprNode *expr = malloc(sizeof(ExprNode));
    if (expr == NULL) {
        perror("malloc failed for subshell");
        exit(1);
    } 

    expr->type = type;
    expr->word.value = value;
    return expr;
}

OpType define_op_type(Token token) {
    switch (token.type) {
        case TOKEN_OR_IF:
            return OP_OR_IF;
        case TOKEN_AND_IF:
            return OP_AND_IF;
        case TOKEN_SEMICOLON:
            return OP_SEMICOLON;
        case TOKEN_PIPE:
            return OP_PIPE;
        case TOKEN_AMPERSAND:
            return OP_AMPERSAND;
        default:
            perror("unknown op");
            exit(1);
    }
}

RedirOp define_redir_op(Token token) {
    switch (token.type) {
        case TOKEN_LESS:
            return REDIR_LESS;
        case TOKEN_GREATER:
            return REDIR_GREATER;
        case TOKEN_GG:
            return REDIR_GG;
        default:
            perror("unknown redir");
            exit(1);
    }
}


// основная функция
ExprNode *parse(Parser *parser) {
    ExprNode *root = parse_list(parser);
    p_expect(parser, TOKEN_EOF);
    return root;
}

/* list        = and_or , { ( ";" | "&" ) , [ and_or ] } ;
список разделяется  операторами ; или &
сначала обрабатываем левое поддерево, потом определяем тип операции,
потом читаем правое поддерево*/
ExprNode *parse_list(Parser *parser) {
    ExprNode *left = parse_and_or(parser);
    while (p_match(parser, TOKEN_SEMICOLON) || p_match(parser, TOKEN_AMPERSAND)) {
        OpType op = define_op_type(p_previous(parser));
        ExprNode *right = NULL;
        if (!p_check(parser, TOKEN_EOF) && !p_check(parser, TOKEN_RPAREN)) {
            right = parse_and_or(parser);
        }
        left = make_binary(NODE_LIST, left, right, op);
    }
    return left;
}

// and_or      = pipeline , { ( "&&" | "||" ) , pipeline } ;
ExprNode *parse_and_or(Parser *parser) {
    ExprNode *left = parse_pipeline(parser);
    while (p_match(parser, TOKEN_AND_IF) || p_match(parser, TOKEN_OR_IF)) {
        OpType op = define_op_type(p_previous(parser));
        ExprNode *right = parse_pipeline(parser);
        if (right == NULL) {
            parser->has_error = 1;
            free_ast(left);
            return NULL;
        }
        left = make_binary(NODE_AND_OR, left, right, op);
    }
    return left;
}

// pipeline    = command , { "|" , command } ;
ExprNode  *parse_pipeline(Parser *parser) {
    ExprNode *left = parse_command(parser);
    while (p_match(parser, TOKEN_PIPE)) {
        OpType op = define_op_type(p_previous(parser));
        ExprNode *right = parse_command(parser);
        left = make_binary(NODE_PIPELINE, left, right, op);
    }
    return left;
}

// command     = simple | subshell ;
ExprNode  *parse_command(Parser *parser) {
    if (p_match(parser, TOKEN_LPAREN)) {
        return parse_subshell(parser);
    }
    return parse_simple(parser);
}

// subshell    = "(" , list , ")" , { redirection } ;
ExprNode  *parse_subshell(Parser *parser) {
    ExprNode *list= parse_list(parser);
    ExprNode *redirs = parse_redirection(parser);
    
    p_expect(parser, TOKEN_RPAREN);
    ExprNode *subshell = make_subshell(NODE_SUBSHELL, list, redirs);
    
    return subshell;
}

// redirection = ( "<" | ">" | ">>" ) , word ;
ExprNode  *parse_redirection(Parser *parser) {
    if (p_match(parser, TOKEN_LESS) || p_match(parser, TOKEN_GREATER) || p_match(parser, TOKEN_GG)) {
        RedirOp op = define_redir_op(p_previous(parser));
        if (!p_check(parser, TOKEN_WORD)) {
            perror("expected filename after redirection");
            exit(1);
        }
        Token fname = *p_advance(parser);
        ExprNode *next = parse_redirection(parser);
        return make_redir(NODE_REDIR, next, fname.value, op);
    }
    return NULL;
}

// сделать редирекции 
// simple      = { redirection } , word , { word | redirection } ;
ExprNode  *parse_simple(Parser *parser) {
    ExprNode *first_redir = parse_redirection(parser);
    
    if (!p_check(parser, TOKEN_WORD)) {
        perror("expected command name");
        exit(1);
    }
    Token command = *p_advance(parser);

    ExprNode *last_redir = first_redir;
    int cap = 4, len = 0;
    char **args = malloc(sizeof(char *) * cap);
    args[len++] = strdup(command.value);

    while (last_redir != NULL && last_redir->redir.next != NULL) {
        last_redir = last_redir->redir.next;
    }
    while (p_check(parser, TOKEN_WORD) || p_check(parser, TOKEN_LESS) ||
            p_check(parser, TOKEN_GREATER) || p_check(parser, TOKEN_GG)) {
        if (p_check(parser, TOKEN_LESS) || p_check(parser, TOKEN_GREATER) || p_check(parser, TOKEN_GG)) {
            ExprNode *redir = parse_redirection(parser);
            if (last_redir == NULL) {
                first_redir = redir;
            } else {
                last_redir->redir.next = redir;
            }
            last_redir = redir;
            while (last_redir->redir.next != NULL) {
                last_redir = last_redir->redir.next;
            }
        } else {
            args[len++] = strdup(p_advance(parser)->value);
            if (len >= cap) {
                cap *= 2;
                args = realloc(args, sizeof(char *) * cap);
            }
        }
    }
    ExprNode *simple = make_simple(NODE_SIMPLE, first_redir, args, len);
    return simple;
}

Token p_peek(Parser *parser, int offset) {
    if (parser->pos + offset >= parser->tokens->len) {
        Token eof_token;
        eof_token.type = TOKEN_EOF;
        eof_token.value = NULL;
        return eof_token;
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

void parser_error(Parser *p, const char *msg) {
    if (!p->has_error) {
        p->has_error = 1;
        fprintf(stderr, "my_shell: syntax error: %s\n", msg);
    }
}

void free_ast(ExprNode *node) {
    if (!node) return;
    switch (node->type) {
        case NODE_LIST:
        case NODE_AND_OR:
        case NODE_PIPELINE:
            free_ast(node->binary.left);
            free_ast(node->binary.right);
            break;
        case NODE_SIMPLE:
            free_ast(node->simple.redirs);
            for (int i = 0; i < node->simple.argc; ++i) {
                free(node->simple.args[i]);
            }
            free(node->simple.args);\
            break;
        case NODE_REDIR:
            free_ast(node->redir.next);
            free(node->redir.fname);
            break;
        case NODE_SUBSHELL:
            free_ast(node->subshell.inner);
            free_ast(node->subshell.redirs);
            break;
        case NODE_ARG:
            free(node->word.value);
            break;
    }
    free(node);
}
