#ifndef DOPPLER_LEXER_H
#define DOPPLER_LEXER_H

#include "error.h"
#include <stddef.h>

typedef enum {

    /* Operators */
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    TOK_EQUAL,
    TOK_EQUAL_EQUAL,
    TOK_BANG_EQUAL,
    TOK_LESS,
    TOK_LESS_EQUAL,
    TOK_GREATER,
    TOK_GREATER_EQUAL,
    TOK_AND,
    TOK_OR,
    TOK_BANG,        /* !a */
    TOK_PLUS_PLUS,   /* a++, only postfix */
    TOK_MINUS_MINUS, /* a--, only postfix */
    TOK_DOT,
    TOK_COMMA,
    TOK_SEMICOLON,

    TOK_IDENT,
    TOK_NUMBERLIT,
    TOK_FLOATLIT,
    TOK_STRINGLIT,
    TOK_CHARLIT,
    TOK_BOOLLIT,

    /* Keywords */
    TOK_BREAK,
    TOK_CASE,
    TOK_CONST,
    TOK_CLASS,
    TOK_DELETE,
    TOK_IF,
    TOK_ELSE,
    TOK_FOR,
    TOK_IMPLEMENTS,
    TOK_EXTENDS,
    TOK_SWITCH,
    TOK_NEW,
    TOK_NULL,
    TOK_VAR,
    TOK_WHILE,
    TOK_RETURN,
    TOK_THIS, /* this pointer */

    /* Type keywords */
    TOK_INT,
    TOK_FLOAT,
    TOK_STRING, /* char*, not Java string */
    TOK_CHAR,
    TOK_BOOL, /* boolean types, true or false */

    /* Annotation: @override, @autorelease, @retain etc */
    TOK_ANNOT,

    /* EOF */
    TOK_EOF
} dp_token_type;

typedef struct {
    const char *spelling;
    dp_token_type type;
} reserved_keyword;

typedef struct {
    dp_token_type type;
    int line, column;
    size_t length;

    union {
        const char *ident;
        int number;
        double float_val;
        const char *string;
        int bool_val;
    } value;
} dp_token;

typedef struct {
    const char *input;
    const char *file;
    int pos, line, column;
} lexer_state;

/**
 * Initialize the lexer state with the input string and its source filename.
 */
void lexer_init(lexer_state *state, const char *input, const char *file);

/**
 * Peek at the next token without advancing the lexer.
 */
dp_error_type lexer_peek(lexer_state *state, dp_token *token);

/**
 * Advance the lexer to the next token. On errors, token->line and
 * token->column identify the location to use with state->file.
 */
dp_error_type lexer_next(lexer_state *state, dp_token *token);
#endif // DOPPLER_LEXER_H