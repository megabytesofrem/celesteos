#include "lexer.h"

#include <stdio.h>

int main(void) {
    lexer_state state;
    dp_token token;

    const char *input = "int x = 42;";
    lexer_init(&state, input, "example.dp");
    while (lexer_next(&state, &token) == ERR_NONE && token.type != TOK_EOF) {
        // Process token
        printf("Token: type=%d, line=%d, column=%d\n", token.type, token.line, token.column);
    }
}