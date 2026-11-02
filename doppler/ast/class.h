#ifndef DOPPLER_AST_CLASS_H
#define DOPPLER_AST_CLASS_H

#include "expression.h"
#include "statement.h"

typedef struct ast_class ast_class;
typedef struct ast_class_member_field ast_class_member_field;
typedef struct ast_class_method_field ast_class_method_field;

/**
 * Represents a class declaration in the AST.
 */
struct ast_class {
    ast_node base; /* isa pointer */
    ast_text class_name;

    ast_class *superclass;           /* isa pointer to superclass */
    ast_class_member_field *members; /* array of class members */
    size_t member_count;             /* number of class members */
    ast_class_method_field *methods; /* array of methods */
    size_t method_count;             /* number of methods */
};

typedef enum {
    AST_ATTRIBUTE_OVERRIDE,        /* @override */
    AST_ATTRIBUTE_RETAIN,          /* @retain */
    AST_ATTRIBUTE_RELEASE,         /* @release */
    AST_ATTRIBUTE_DRAIN,           /* @drain */
    AST_ATTRIBUTE_AUTORELEASEPOOL, /* @autoreleasepool { ..  }*/
} ast_attribute_kind;

typedef struct {
    ast_node base; /* isa pointer */
    ast_attribute_kind kind;
} ast_attribute;

typedef struct {
    ast_node base; /* isa pointer */
    ast_text parameter_name;
    ast_type_kind parameter_type;
} ast_method_parameter;

struct ast_class_member_field {
    ast_node base; /* isa pointer */
    ast_text member_name;
    ast_type_kind member_type;
};

struct ast_class_method_field {
    ast_node base; /* isa pointer */
    ast_text method_name;

    ast_method_parameter *parameters; /* array of parameters */
    size_t parameter_count;           /* number of parameters */
    ast_type_kind return_type;        /* return type of the method */
};

typedef struct {
    ast_node base;                  /* isa pointer */
    ast_class_method_field *method; /* pointer to the method being implemented */
    ast_statement *body;            /* pointer to the method body */
} ast_class_method_impl;

void ast_class_init(ast_class *node, dp_error_location location, ast_text class_name,
                    ast_class *superclass, ast_class_member_field *members, size_t member_count,
                    ast_class_method_field *methods, size_t method_count);
void ast_attribute_init(ast_attribute *node, dp_error_location location, ast_attribute_kind kind);
void ast_method_parameter_init(ast_method_parameter *node, dp_error_location location,
                               ast_text parameter_name, ast_type_kind parameter_type);
void ast_class_member_field_init(ast_class_member_field *node, dp_error_location location,
                                 ast_text member_name, ast_type_kind member_type);
void ast_class_method_field_init(ast_class_method_field *node, dp_error_location location,
                                 ast_text method_name, ast_method_parameter *parameters,
                                 size_t parameter_count, ast_type_kind return_type);
void ast_class_method_impl_init(ast_class_method_impl *node, dp_error_location location,
                                ast_class_method_field *method, ast_statement *body);

#endif // DOPPLER_AST_CLASS_H