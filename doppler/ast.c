#include "ast/ast_expression.h"
#include "ast/ast_literal.h"

void ast_identifier_init(ast_identifier *node, dp_error_location location, ast_text name) {
    *node = (ast_identifier) {
        .base.base = { .type = AST_NODE_IDENT, .location = location },
        .name = name,
    };
}

void ast_binary_init(ast_binary *node, dp_error_location location, dp_token_type operator,
                     ast_expression *left, ast_expression *right) {
    *node = (ast_binary) {
        .base.base = { .type = AST_NODE_BINARY, .location = location },
        .operator = operator,
        .left = left,
        .right = right,
    };
}

void ast_unary_init(ast_unary *node, dp_error_location location, dp_token_type operator,
                    ast_expression *operand, bool postfix) {
    *node = (ast_unary) {
        .base.base = { .type = AST_NODE_UNARY, .location = location },
        .operator = operator,
        .operand = operand,
        .postfix = postfix,
    };
}

void ast_number_init(ast_literal *node, dp_error_location location, int value) {
    *node = (ast_literal) {
        .base.base = { .type = AST_NODE_NUMBERLIT, .location = location },
        .value.int_val = value,
    };
}

void ast_float_init(ast_literal *node, dp_error_location location, double value) {
    *node = (ast_literal) {
        .base.base = { .type = AST_NODE_FLOATLIT, .location = location },
        .value.float_val = value,
    };
}

void ast_string_init(ast_literal *node, dp_error_location location, ast_text value) {
    *node = (ast_literal) {
        .base.base = { .type = AST_NODE_STRINGLIT, .location = location },
        .value.string_val = value,
    };
}

void ast_char_init(ast_literal *node, dp_error_location location, char value) {
    *node = (ast_literal) {
        .base.base = { .type = AST_NODE_CHARLIT, .location = location },
        .value.char_val = value,
    };
}

void ast_bool_init(ast_literal *node, dp_error_location location, bool value) {
    *node = (ast_literal) {
        .base.base = { .type = AST_NODE_BOOLLIT, .location = location },
        .value.bool_val = value,
    };
}

void ast_null_init(ast_literal *node, dp_error_location location) {
    *node = (ast_literal) {
        .base.base = { .type = AST_NODE_NULLLIT, .location = location },
    };
}
