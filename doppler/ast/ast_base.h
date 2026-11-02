#ifndef DOPPLER_AST_BASE_H
#define DOPPLER_AST_BASE_H

#include "../error.h"

typedef enum {
    AST_NODE_IDENTIFIER,
    AST_NODE_NUMBER_LITERAL,
    AST_NODE_FLOAT_LITERAL,
    AST_NODE_STRING_LITERAL,
    AST_NODE_CHAR_LITERAL,
    AST_NODE_BOOL_LITERAL,
    AST_NODE_NULL_LITERAL,

    AST_NODE_BINARY,
    AST_NODE_UNARY,
    AST_NODE_STATEMENT,
    AST_NODE_CLASS,
    AST_NODE_METHOD,
    AST_NODE_METHOD_IMPL,
    AST_NODE_EXPRESSION_STATEMENT,
    AST_NODE_VAR_DECLARE_STATEMENT,
    AST_NODE_ASSIGNMENT_STATEMENT,
    AST_NODE_RETURN_STATEMENT,
    AST_NODE_IF_STATEMENT,
    AST_NODE_FOR_STATEMENT,
    AST_NODE_WHILE_STATEMENT,
    AST_NODE_ATTRIBUTE,
    AST_NODE_METHOD_PARAMETER,
    AST_NODE_CLASS_MEMBER_FIELD
} ast_node_type;

typedef struct ast_node {
    ast_node_type type;
    dp_error_location location;
} ast_node;

/*
 * Initializers fully replace valid caller-provided node storage. Child pointers,
 * text, filenames, and arrays are borrowed and must outlive the node. Arrays
 * may be NULL when their counts are zero; no initializer allocates or frees.
 */

#endif // DOPPLER_AST_BASE_H