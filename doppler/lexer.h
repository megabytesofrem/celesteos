#ifndef DOPPLER_LEXER_H
#define DOPPLER_LEXER_H

#include "error.h"
#include <stddef.h>

typedef enum {
    TOK_IDENT,
    TOK_NUMBER,
    TOK_FLOAT,
    TOK_STRING,
    TOK_BOOL,

    // Keywords
    TOK_CASE,
    TOK_CLASS,
    TOK_DO,
    TOK_IF,
    TOK_IN,
    TOK_ELSE,
    TOK_ELIF,
    TOK_FOR,
    TOK_IMPLEMENTS,
    TOK_LET,
    TOK_THEN,
    TOK_WHILE,
    TOK_RETURN,

    // EOF
    TOK_EOF
} dp_token_type;

typedef struct {
    dp_token_type type;
    int line, column;
    size_t length;

    union {
        char *ident;
        int number;
        double float_val;
        const char *string;
        int bool_val;
    } value;
} dp_token;

typedef struct {
    const char *input;
    int pos, line, column;
} lexer_state;

/**
 * Initialize the lexer state with the given input string.
 */
void lexer_init(lexer_state *state, const char *input);

/**
 * Peek at the next token without advancing the lexer.
 */
dp_error_type lexer_peek(lexer_state *state, dp_token *token);

/**
 * Advance the lexer to the next token, storing the result in the out token parameter.
 */
dp_error_type lexer_next(lexer_state *state, dp_token *token);
#endif // DOPPLER_LEXER_H