#pragma once

#include "ast.h"

int execute(ExprNode *node, int bg);
int execute_list(ExprNode *node, int bg);
int execute_and_or(ExprNode *node, int bg);
int execute_pipeline(ExprNode *node, int bg);
int execute_simple(ExprNode *node, int bg);
int execute_subshell(ExprNode *node, int bg);
