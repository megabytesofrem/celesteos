#ifndef DOPPLER_AST_BASE_H
#define DOPPLER_AST_BASE_H

#include "error.h"

typedef enum {
    AST_NODE_IDENT,

    /* Literal nodes */
    AST_NODE_NUMBERLIT,
    AST_NODE_FLOATLIT,
    AST_NODE_STRINGLIT,
    AST_NODE_CHARLIT,
    AST_NODE_BOOLLIT,
    AST_NODE_NULLLIT,

    /* Expression nodes */
    AST_NODE_BINARY,
    AST_NODE_UNARY,
} ast_node_type;

typedef struct ast_node {
    ast_node_type type;
    dp_error_location location;
} ast_node;

#endif // DOPPLER_AST_BASE_H