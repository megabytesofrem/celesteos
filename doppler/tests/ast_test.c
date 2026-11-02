#include "../ast/ast_expression.h"
#include "../ast/ast_literal.h"
#include <assert.h>
#include <string.h>

static int evaluate(const ast_expression *expression) {
    switch (expression->base.type) {
    case AST_NODE_NUMBERLIT:
        return ((const ast_literal *)expression)->value.int_val;
    case AST_NODE_BINARY: {
        const ast_binary *binary = (const ast_binary *)expression;
        assert(binary->operator == TOK_PLUS);
        return evaluate(binary->left) + evaluate(binary->right);
    }
    case AST_NODE_UNARY: {
        const ast_unary *unary = (const ast_unary *)expression;
        assert(unary->operator == TOK_MINUS && !unary->postfix);
        return -evaluate(unary->operand);
    }
    default:
        assert(0);
        return 0;
    }
}

int main(void) {
    dp_error_location location = { "test.dp", 2, 3 };
    ast_literal left, right, literal;
    ast_binary sum;
    ast_unary negative;
    ast_identifier identifier;
    const char text[] = { 'a', '\0', 'b' };

    ast_number_init(&left, location, 2);
    ast_number_init(&right, location, 3);
    ast_binary_init(&sum, location, TOK_PLUS, &left.base, &right.base);
    ast_unary_init(&negative, location, TOK_MINUS, &sum.base, false);
    assert(evaluate(&negative.base) == -5);
    assert(negative.base.base.location.line == 2);
    assert(negative.base.base.location.column == 3);
    assert(strcmp(negative.base.base.location.file, "test.dp") == 0);

    ast_identifier_init(&identifier, location, (ast_text){ "name", 4 });
    assert(identifier.base.base.type == AST_NODE_IDENT);
    assert(identifier.name.length == 4);
    assert(memcmp(identifier.name.data, "name", 4) == 0);
    ast_unary_init(&negative, location, TOK_PLUS_PLUS, &identifier.base, true);
    assert(negative.postfix && negative.operand == &identifier.base);

    ast_float_init(&literal, location, 1.25);
    assert(literal.base.base.type == AST_NODE_FLOATLIT && literal.value.float_val == 1.25);
    ast_string_init(&literal, location, (ast_text){ text, sizeof(text) });
    assert(literal.base.base.type == AST_NODE_STRINGLIT);
    assert(literal.value.string_val.data == text);
    assert(literal.value.string_val.length == sizeof(text));
    ast_char_init(&literal, location, 'x');
    assert(literal.base.base.type == AST_NODE_CHARLIT && literal.value.char_val == 'x');
    ast_bool_init(&literal, location, true);
    assert(literal.base.base.type == AST_NODE_BOOLLIT && literal.value.bool_val);
    ast_bool_init(&literal, location, false);
    assert(!literal.value.bool_val);
    ast_null_init(&literal, location);
    assert(literal.base.base.type == AST_NODE_NULLLIT);
}
