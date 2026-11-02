#ifndef DOPPLER_AST_EXPRESSION_H
#define DOPPLER_AST_EXPRESSION_H

#include "../lexer.h"
#include "ast_base.h"

#include <stdbool.h>
#include <stddef.h>

/**
 * Abstract syntax tree (AST) expression node.
 */
typedef struct ast_expression {
    ast_node base; /* isa pointer */
} ast_expression;

typedef struct {
    const char *data;
    size_t length;
} ast_text;

typedef enum {
    AST_TYPE_INT,
    AST_TYPE_FLOAT,
    AST_TYPE_BOOL,
    AST_TYPE_STRING,
    AST_TYPE_VOID
} ast_type_kind;

typedef struct {
    ast_expression base; /* isa pointer */
    ast_text name;
} ast_identifier;

typedef struct {
    ast_expression base; /* isa pointer */
    dp_token_type operator;
    ast_expression *left;
    ast_expression *right;
} ast_binary;

typedef struct {
    ast_expression base; /* isa pointer */
    dp_token_type operator;
    ast_expression *operand;
    bool postfix;
} ast_unary;

/* Nodes, child expressions, text, and location filenames remain caller-owned. */
void ast_identifier_init(ast_identifier *node, dp_error_location location, ast_text name);
void ast_binary_init(ast_binary *node, dp_error_location location, dp_token_type operator,
                     ast_expression * left, ast_expression *right);
void ast_unary_init(ast_unary *node, dp_error_location location, dp_token_type operator,
                    ast_expression * operand, bool postfix);

#endif // DOPPLER_AST_EXPRESSION_H
