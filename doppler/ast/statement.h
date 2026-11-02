#ifndef DOPPLER_AST_STATEMENT_H
#define DOPPLER_AST_STATEMENT_H

#include "expression.h"

/**
 * Abstract syntax tree (AST) statement node.
 */
typedef struct ast_statement {
    ast_node base; /* isa pointer */
} ast_statement;

typedef struct {
    ast_statement base;         /* isa pointer */
    ast_expression *expression; /* pointer to the expression */
} ast_expression_statement;

typedef struct {
    ast_statement base; /* isa pointer */
    ast_text var_name;
    ast_type_kind var_type;
    ast_expression *initializer; /* pointer to the initializer expression, if any */
} ast_var_declare_statement;

typedef struct {
    ast_statement base; /* isa pointer */
    ast_expression *lhs;
    ast_expression *rhs;
} ast_assignment_statement;

typedef struct {
    ast_statement base;         /* isa pointer */
    ast_expression *expression; /* pointer to the return expression, if any */
} ast_return_statement;

typedef struct {
    ast_statement base; /* isa pointer */
    ast_expression *condition;
    ast_statement *then_branch;
    ast_statement *else_branch; /* pointer to the else branch statement, if any */
} ast_if_statement;

typedef struct {
    ast_statement base; /* isa pointer */
    ast_statement *initializer; /* optional */
    ast_expression *condition; /* optional */
    ast_expression *increment; /* optional */
    ast_statement *body;
} ast_for_statement;

typedef struct {
    ast_statement base; /* isa pointer */
    ast_expression *condition;
    ast_statement *body;
} ast_while_statement;

void ast_expression_statement_init(ast_expression_statement *node, dp_error_location location,
                                   ast_expression *expression);
void ast_var_declare_statement_init(ast_var_declare_statement *node, dp_error_location location,
                                    ast_text var_name, ast_type_kind var_type,
                                    ast_expression *initializer);
void ast_assignment_statement_init(ast_assignment_statement *node, dp_error_location location,
                                   ast_expression *lhs, ast_expression *rhs);
void ast_return_statement_init(ast_return_statement *node, dp_error_location location,
                               ast_expression *expression);
void ast_if_statement_init(ast_if_statement *node, dp_error_location location,
                           ast_expression *condition, ast_statement *then_branch,
                           ast_statement *else_branch);
void ast_for_statement_init(ast_for_statement *node, dp_error_location location,
                            ast_statement *initializer, ast_expression *condition,
                            ast_expression *increment, ast_statement *body);
void ast_while_statement_init(ast_while_statement *node, dp_error_location location,
                              ast_expression *condition, ast_statement *body);

#endif // DOPPLER_AST_STATEMENT_H