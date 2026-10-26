#pragma once

#include "ast.h"

int execute(ExprNode *node, int bg, char *cmd_line);
int execute_list(ExprNode *node, int bg, char *cmd_line);
int execute_and_or(ExprNode *node, int bg, char *cmd_line);
int execute_pipeline(ExprNode *node, int bg, char *cmd_line);
int execute_simple(ExprNode *node, int bg, char *cmd_line);
int execute_subshell(ExprNode *node, int bg, char *cmd_line);
