#include "ast_nodes.h"

/* Literal and expression initializers */

void ast_number_init(ast_literal *node, dp_error_location location, int value) {
    *node = (ast_literal){
        .base.base = { .type = AST_NODE_NUMBER_LITERAL, .location = location },
        .value.int_val = value,
    };
}

void ast_float_init(ast_literal *node, dp_error_location location, double value) {
    *node = (ast_literal){
        .base.base = { .type = AST_NODE_FLOAT_LITERAL, .location = location },
        .value.float_val = value,
    };
}

void ast_string_init(ast_literal *node, dp_error_location location, ast_text value) {
    *node = (ast_literal){
        .base.base = { .type = AST_NODE_STRING_LITERAL, .location = location },
        .value.string_val = value,
    };
}

void ast_char_init(ast_literal *node, dp_error_location location, char value) {
    *node = (ast_literal){
        .base.base = { .type = AST_NODE_CHAR_LITERAL, .location = location },
        .value.char_val = value,
    };
}

void ast_bool_init(ast_literal *node, dp_error_location location, bool value) {
    *node = (ast_literal){
        .base.base = { .type = AST_NODE_BOOL_LITERAL, .location = location },
        .value.bool_val = value,
    };
}

void ast_null_init(ast_literal *node, dp_error_location location) {
    *node = (ast_literal){
        .base.base = { .type = AST_NODE_NULL_LITERAL, .location = location },
    };
}

void ast_identifier_init(ast_identifier *node, dp_error_location location, ast_text name) {
    *node = (ast_identifier){
        .base.base = { .type = AST_NODE_IDENTIFIER, .location = location },
        .name = name,
    };
}

void ast_binary_init(ast_binary *node, dp_error_location location, dp_token_type operator,
                     ast_expression *left, ast_expression *right) {
    *node = (ast_binary){
        .base.base = { .type = AST_NODE_BINARY, .location = location },
        .operator = operator,
        .left = left,
        .right = right,
    };
}

void ast_unary_init(ast_unary *node, dp_error_location location, dp_token_type operator,
                    ast_expression *operand, bool postfix) {
    *node = (ast_unary){
        .base.base = { .type = AST_NODE_UNARY, .location = location },
        .operator = operator,
        .operand = operand,
        .postfix = postfix,
    };
}

/* Statement initializers */

void ast_expression_statement_init(ast_expression_statement *node, dp_error_location location,
                                   ast_expression *expression) {
    *node = (ast_expression_statement){
        .base.base = { .type = AST_NODE_EXPRESSION_STATEMENT, .location = location },
        .expression = expression,
    };
}

void ast_var_declare_statement_init(ast_var_declare_statement *node, dp_error_location location,
                                    ast_text var_name, ast_type_kind var_type,
                                    ast_expression *initializer) {
    *node = (ast_var_declare_statement){
        .base.base = { .type = AST_NODE_VAR_DECLARE_STATEMENT, .location = location },
        .var_name = var_name,
        .var_type = var_type,
        .initializer = initializer,
    };
}

void ast_assignment_statement_init(ast_assignment_statement *node, dp_error_location location,
                                   ast_expression *lhs, ast_expression *rhs) {
    *node = (ast_assignment_statement){
        .base.base = { .type = AST_NODE_ASSIGNMENT_STATEMENT, .location = location },
        .lhs = lhs,
        .rhs = rhs,
    };
}

void ast_return_statement_init(ast_return_statement *node, dp_error_location location,
                               ast_expression *expression) {
    *node = (ast_return_statement){
        .base.base = { .type = AST_NODE_RETURN_STATEMENT, .location = location },
        .expression = expression,
    };
}

void ast_if_statement_init(ast_if_statement *node, dp_error_location location,
                           ast_expression *condition, ast_statement *then_branch,
                           ast_statement *else_branch) {
    *node = (ast_if_statement){
        .base.base = { .type = AST_NODE_IF_STATEMENT, .location = location },
        .condition = condition,
        .then_branch = then_branch,
        .else_branch = else_branch,
    };
}

void ast_for_statement_init(ast_for_statement *node, dp_error_location location,
                            ast_statement *initializer, ast_expression *condition,
                            ast_expression *increment, ast_statement *body) {
    *node = (ast_for_statement){
        .base.base = { .type = AST_NODE_FOR_STATEMENT, .location = location },
        .initializer = initializer,
        .condition = condition,
        .increment = increment,
        .body = body,
    };
}

void ast_while_statement_init(ast_while_statement *node, dp_error_location location,
                              ast_expression *condition, ast_statement *body) {
    *node = (ast_while_statement){
        .base.base = { .type = AST_NODE_WHILE_STATEMENT, .location = location },
        .condition = condition,
        .body = body,
    };
}

/* Class and class member initializers */

void ast_class_init(ast_class *node, dp_error_location location, ast_text class_name,
                    ast_class *superclass, ast_class_member_field *members, size_t member_count,
                    ast_class_method_field *methods, size_t method_count) {
    *node = (ast_class){
        .base = { .type = AST_NODE_CLASS, .location = location },
        .class_name = class_name,
        .superclass = superclass,
        .members = members,
        .member_count = member_count,
        .methods = methods,
        .method_count = method_count,
    };
}

void ast_attribute_init(ast_attribute *node, dp_error_location location, ast_attribute_kind kind) {
    *node = (ast_attribute){
        .base = { .type = AST_NODE_ATTRIBUTE, .location = location },
        .kind = kind,
    };
}

void ast_method_parameter_init(ast_method_parameter *node, dp_error_location location,
                               ast_text parameter_name, ast_type_kind parameter_type) {
    *node = (ast_method_parameter){
        .base = { .type = AST_NODE_METHOD_PARAMETER, .location = location },
        .parameter_name = parameter_name,
        .parameter_type = parameter_type,
    };
}

void ast_class_member_field_init(ast_class_member_field *node, dp_error_location location,
                                 ast_text member_name, ast_type_kind member_type) {
    *node = (ast_class_member_field){
        .base = { .type = AST_NODE_CLASS_MEMBER_FIELD, .location = location },
        .member_name = member_name,
        .member_type = member_type,
    };
}

void ast_class_method_field_init(ast_class_method_field *node, dp_error_location location,
                                 ast_text method_name, ast_method_parameter *parameters,
                                 size_t parameter_count, ast_type_kind return_type) {
    *node = (ast_class_method_field){
        .base = { .type = AST_NODE_METHOD, .location = location },
        .method_name = method_name,
        .parameters = parameters,
        .parameter_count = parameter_count,
        .return_type = return_type,
    };
}

void ast_class_method_impl_init(ast_class_method_impl *node, dp_error_location location,
                                ast_class_method_field *method, ast_statement *body) {
    *node = (ast_class_method_impl){
        .base = { .type = AST_NODE_METHOD_IMPL, .location = location },
        .method = method,
        .body = body,
    };
}