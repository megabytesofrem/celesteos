#ifndef DOPPLER_AST_EXPRESSION_H
#define DOPPLER_AST_EXPRESSION_H

#include "../ast_base.h"
#include "../lexer.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct ast_expression {
    ast_node base;
} ast_expression;

typedef struct {
    const char *data;
    size_t length;
} ast_text;

typedef struct {
    ast_expression base;
    ast_text name;
} ast_identifier;

typedef struct {
    ast_expression base;
    dp_token_type operator;
    ast_expression *left;
    ast_expression *right;
} ast_binary;

typedef struct {
    ast_expression base;
    dp_token_type operator;
    ast_expression *operand;
    bool postfix;
} ast_unary;

/* Nodes, child expressions, text, and location filenames remain caller-owned. */
void ast_identifier_init(ast_identifier *node, dp_error_location location, ast_text name);
void ast_binary_init(ast_binary *node, dp_error_location location, dp_token_type operator,
                     ast_expression *left, ast_expression *right);
void ast_unary_init(ast_unary *node, dp_error_location location, dp_token_type operator,
                    ast_expression *operand, bool postfix);

#endif // DOPPLER_AST_EXPRESSION_H
