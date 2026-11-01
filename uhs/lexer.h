#ifndef UHS_LEXER_H
#define UHS_LEXER_H

typedef enum {
    TOK_IDENT,
    TOK_NUMBER,
    TOK_FLOAT,
    TOK_STRING,
    TOK_EOF
} uhs_token_type;

#endif // UHS_LEXER_H