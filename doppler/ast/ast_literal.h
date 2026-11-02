#ifndef DOPPLER_AST_LITERAL_H
#define DOPPLER_AST_LITERAL_H

#include "ast_expression.h"

typedef struct {
    ast_expression base;

    union {
        int int_val;
        double float_val;
        ast_text string_val;
        char char_val;
        bool bool_val;
    } value;

} ast_literal;

void ast_number_init(ast_literal *node, dp_error_location location, int value);
void ast_float_init(ast_literal *node, dp_error_location location, double value);
void ast_string_init(ast_literal *node, dp_error_location location, ast_text value);
void ast_char_init(ast_literal *node, dp_error_location location, char value);
void ast_bool_init(ast_literal *node, dp_error_location location, bool value);
void ast_null_init(ast_literal *node, dp_error_location location);

#endif // DOPPLER_AST_LITERAL_H