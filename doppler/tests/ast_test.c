#include "../ast/class.h"
#include "../ast/literal.h"
#include <assert.h>
#include <string.h>

static void check_node(const ast_node *node, ast_node_type type) {
    assert(node->type == type);
    assert(strcmp(node->location.file, "test.dp") == 0);
    assert(node->location.line == 2 && node->location.column == 3);
}

int main(void) {
    dp_error_location loc = { "test.dp", 2, 3 };
    ast_text name = { "name", 4 };
    ast_literal literal;
    ast_identifier ident;
    ast_binary binary;
    ast_unary unary;
    ast_expression_statement expression;
    ast_var_declare_statement variable;
    ast_assignment_statement assignment;
    ast_return_statement result;
    ast_if_statement branch;
    ast_for_statement for_loop;
    ast_while_statement while_loop;
    ast_class parent, child;
    ast_attribute attribute;
    ast_method_parameter parameter;
    ast_class_member_field member;
    ast_class_method_field method;
    ast_class_method_impl implementation;

    ast_number_init(&literal, loc, 42);
    check_node(&literal.base.base, AST_NODE_NUMBER_LITERAL);
    assert(literal.value.int_val == 42);
    ast_float_init(&literal, loc, 1.25);
    check_node(&literal.base.base, AST_NODE_FLOAT_LITERAL);
    assert(literal.value.float_val == 1.25);
    ast_string_init(&literal, loc, name);
    check_node(&literal.base.base, AST_NODE_STRING_LITERAL);
    assert(literal.value.string_val.data == name.data && literal.value.string_val.length == 4);
    ast_char_init(&literal, loc, 'a');
    check_node(&literal.base.base, AST_NODE_CHAR_LITERAL);
    assert(literal.value.char_val == 'a');
    ast_bool_init(&literal, loc, true);
    check_node(&literal.base.base, AST_NODE_BOOL_LITERAL);
    assert(literal.value.bool_val);
    ast_bool_init(&literal, loc, false);
    assert(!literal.value.bool_val);
    ast_null_init(&literal, loc);
    check_node(&literal.base.base, AST_NODE_NULL_LITERAL);

    ast_identifier_init(&ident, loc, name);
    check_node(&ident.base.base, AST_NODE_IDENTIFIER);
    assert(ident.name.data == name.data && ident.name.length == 4);
    ast_binary_init(&binary, loc, TOK_PLUS, &ident.base, &literal.base);
    check_node(&binary.base.base, AST_NODE_BINARY);
    assert(binary.operator == TOK_PLUS && binary.left == &ident.base);
    assert(binary.right == &literal.base);
    ast_unary_init(&unary, loc, TOK_PLUS_PLUS, &ident.base, true);
    check_node(&unary.base.base, AST_NODE_UNARY);
    assert(unary.operator == TOK_PLUS_PLUS && unary.operand == &ident.base && unary.postfix);
    ast_unary_init(&unary, loc, TOK_MINUS, &binary.base, false);
    assert(unary.operator == TOK_MINUS && unary.operand == &binary.base && !unary.postfix);

    ast_expression_statement_init(&expression, loc, &binary.base);
    check_node(&expression.base.base, AST_NODE_EXPRESSION_STATEMENT);
    assert(expression.expression == &binary.base);
    ast_var_declare_statement_init(&variable, loc, name, AST_TYPE_INT, &literal.base);
    check_node(&variable.base.base, AST_NODE_VAR_DECLARE_STATEMENT);
    assert(variable.var_name.data == name.data && variable.var_name.length == 4);
    assert(variable.var_type == AST_TYPE_INT && variable.initializer == &literal.base);
    ast_var_declare_statement_init(&variable, loc, name, AST_TYPE_INT, NULL);
    assert(variable.initializer == NULL);
    ast_assignment_statement_init(&assignment, loc, &ident.base, &literal.base);
    check_node(&assignment.base.base, AST_NODE_ASSIGNMENT_STATEMENT);
    assert(assignment.lhs == &ident.base && assignment.rhs == &literal.base);
    ast_return_statement_init(&result, loc, &literal.base);
    check_node(&result.base.base, AST_NODE_RETURN_STATEMENT);
    assert(result.expression == &literal.base);
    ast_return_statement_init(&result, loc, NULL);
    assert(result.expression == NULL);
    ast_if_statement_init(&branch, loc, &ident.base, &expression.base, &result.base);
    check_node(&branch.base.base, AST_NODE_IF_STATEMENT);
    assert(branch.condition == &ident.base && branch.then_branch == &expression.base);
    assert(branch.else_branch == &result.base);
    ast_if_statement_init(&branch, loc, &ident.base, &expression.base, NULL);
    assert(branch.else_branch == NULL);
    ast_for_statement_init(&for_loop, loc, &variable.base, &ident.base, &unary.base, &branch.base);
    check_node(&for_loop.base.base, AST_NODE_FOR_STATEMENT);
    assert(for_loop.initializer == &variable.base && for_loop.condition == &ident.base);
    assert(for_loop.increment == &unary.base && for_loop.body == &branch.base);
    ast_for_statement_init(&for_loop, loc, NULL, NULL, NULL, &branch.base);
    assert(!for_loop.initializer && !for_loop.condition && !for_loop.increment);
    ast_while_statement_init(&while_loop, loc, &ident.base, &expression.base);
    check_node(&while_loop.base.base, AST_NODE_WHILE_STATEMENT);
    assert(while_loop.condition == &ident.base && while_loop.body == &expression.base);

    ast_attribute_init(&attribute, loc, AST_ATTRIBUTE_OVERRIDE);
    check_node(&attribute.base, AST_NODE_ATTRIBUTE);
    assert(attribute.kind == AST_ATTRIBUTE_OVERRIDE);
    ast_method_parameter_init(&parameter, loc, name, AST_TYPE_BOOL);
    check_node(&parameter.base, AST_NODE_METHOD_PARAMETER);
    assert(parameter.parameter_name.data == name.data && parameter.parameter_name.length == 4);
    assert(parameter.parameter_type == AST_TYPE_BOOL);
    ast_class_member_field_init(&member, loc, name, AST_TYPE_STRING);
    check_node(&member.base, AST_NODE_CLASS_MEMBER_FIELD);
    assert(member.member_name.data == name.data && member.member_name.length == 4);
    assert(member.member_type == AST_TYPE_STRING);
    ast_class_method_field_init(&method, loc, name, &parameter, 1, AST_TYPE_VOID);
    check_node(&method.base, AST_NODE_METHOD);
    assert(method.method_name.data == name.data && method.method_name.length == 4);
    assert(method.parameters == &parameter && method.parameter_count == 1);
    assert(method.return_type == AST_TYPE_VOID);
    ast_class_method_impl_init(&implementation, loc, &method, &result.base);
    check_node(&implementation.base, AST_NODE_METHOD_IMPL);
    assert(implementation.method == &method && implementation.body == &result.base);
    ast_class_init(&parent, loc, name, NULL, NULL, 0, NULL, 0);
    check_node(&parent.base, AST_NODE_CLASS);
    assert(!parent.superclass && !parent.members && !parent.methods);
    assert(parent.member_count == 0 && parent.method_count == 0);
    ast_class_init(&child, loc, name, &parent, &member, 1, &method, 1);
    check_node(&child.base, AST_NODE_CLASS);
    assert(child.class_name.data == name.data && child.class_name.length == 4);
    assert(child.superclass == &parent && child.members == &member && child.member_count == 1);
    assert(child.methods == &method && child.method_count == 1);
}
