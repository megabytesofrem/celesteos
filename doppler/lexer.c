#include "lexer.h"

#include <stdbool.h>
#include <string.h>

#define NUM_KEYWORDS (sizeof(reserved_keywords) / sizeof(reserved_keywords[0]))

void lexer_init(lexer_state *state, const char *input, const char *file) {
    state->input = input;
    state->file = file;
    state->pos = 0;
    state->line = 1;
    state->column = 1;
}

static void skip_ws(lexer_state *state);

static dp_error_type lex_number(lexer_state *state, dp_token *token);
static dp_error_type lex_float(lexer_state *state, dp_token *token);
static dp_error_type lex_string(lexer_state *state, dp_token *token);
static dp_error_type lex_char(lexer_state *state, dp_token *token);
static dp_error_type lex_ident(lexer_state *state, dp_token *token);
static bool is_digit(char c);
static bool is_alpha(char c);
static void lexer_advance(lexer_state *state);
static void accept(lexer_state *state, dp_token_type tok, dp_token *token);

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static void accept(lexer_state *state, dp_token_type tok, dp_token *token) {
    token->type = tok;
    token->length++;
    lexer_advance(state);
}

/**
 * Map reserved words to their token types.
 */
static const reserved_keyword reserved_keywords[] = {
    { "break", TOK_BREAK },     { "case", TOK_CASE },
    { "class", TOK_CLASS },     { "const", TOK_CONST },
    { "if", TOK_IF },           { "else", TOK_ELSE },
    { "for", TOK_FOR },         { "implements", TOK_IMPLEMENTS },
    { "extends", TOK_EXTENDS }, { "switch", TOK_SWITCH },
    { "var", TOK_VAR },         { "while", TOK_WHILE },
    { "return", TOK_RETURN },   { "int", TOK_INT },
    { "float", TOK_FLOAT },     { "string", TOK_STRING },
    { "char", TOK_CHAR },       { "bool", TOK_BOOL },
    { "true", TOK_BOOLLIT },    { "false", TOK_BOOLLIT },
};

/* */

dp_error_type lexer_next(lexer_state *state, dp_token *token) {
    skip_ws(state);

    token->line = state->line;
    token->column = state->column;
    token->length = 0;

    switch (state->input[state->pos]) {
    case '\0':
        token->type = TOK_EOF;
        return ERR_NONE;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return lex_number(state, token);
    case '"':
        return lex_string(state, token);
    case '\'':
        return lex_char(state, token);
    case '+':
        accept(state, TOK_PLUS, token);
        if (state->input[state->pos] == '+') {
            accept(state, TOK_PLUS_PLUS, token);
        }
        return 0;
    case '-':
        accept(state, TOK_MINUS, token);
        if (state->input[state->pos] == '-') {
            accept(state, TOK_MINUS_MINUS, token);
        }
        return ERR_NONE;
    case '*':
        accept(state, TOK_STAR, token);
        return ERR_NONE;
    case '/':
        accept(state, TOK_SLASH, token);
        return ERR_NONE;
    case '%':
        accept(state, TOK_PERCENT, token);
        return ERR_NONE;
    case '=':
        accept(state, TOK_EQUAL, token);
        if (state->input[state->pos] == '=') {
            accept(state, TOK_EQUAL_EQUAL, token);
        }
        return ERR_NONE;
    case '!':
        accept(state, TOK_BANG, token);
        if (state->input[state->pos] == '=') {
            accept(state, TOK_BANG_EQUAL, token);
        }
        return ERR_NONE;
    case '<':
        accept(state, TOK_LESS, token);
        if (state->input[state->pos] == '=') {
            accept(state, TOK_LESS_EQUAL, token);
        }
        return ERR_NONE;
    case '>':
        accept(state, TOK_GREATER, token);
        if (state->input[state->pos] == '=') {
            accept(state, TOK_GREATER_EQUAL, token);
        }
        return ERR_NONE;
    case '&':
        lexer_advance(state);
        if (state->input[state->pos] != '&') {
            return ERR_UNEXPECTED_CHAR;
        }
        accept(state, TOK_AND, token);
        token->length = 2;
        return ERR_NONE;
    case '|':
        lexer_advance(state);
        if (state->input[state->pos] != '|') {
            return ERR_UNEXPECTED_CHAR;
        }
        accept(state, TOK_OR, token);
        token->length = 2;
        return ERR_NONE;
    case '.':
        accept(state, TOK_DOT, token);
        return ERR_NONE;
    case ',':
        accept(state, TOK_COMMA, token);
        return ERR_NONE;
    case ';':
        accept(state, TOK_SEMICOLON, token);
        return ERR_NONE;
    default:
        if (is_alpha(state->input[state->pos])) {
            return lex_ident(state, token);
        }

        // Advance past the unexpected character
        lexer_advance(state);

        // Report the unexpected character as an error
        return ERR_UNEXPECTED_CHAR;
    }
}

dp_error_type lexer_peek(lexer_state *state, dp_token *token) {
    lexer_state lookahead = *state;
    return lexer_next(&lookahead, token);
}

static void lexer_advance(lexer_state *state) {
    state->pos++;
    state->column++;
}

static void skip_ws(lexer_state *state) {
    while (state->input[state->pos] == ' ' || state->input[state->pos] == '\t' ||
           state->input[state->pos] == '\n' || state->input[state->pos] == '\r') {
        if (state->input[state->pos] == '\n') {
            state->line++;
            state->column = 1;
        } else {
            state->column++;
        }
        state->pos++;
    }
}

static dp_error_type lex_number(lexer_state *state, dp_token *token) {
    token->value.number = 0;

    // Check if the number contains a decimal point, indicating a float.
    int start_pos = state->pos;
    while (is_digit(state->input[state->pos])) {
        token->value.number = token->value.number * 10 + (state->input[state->pos] - '0');
        lexer_advance(state);
    }

    if (state->input[state->pos] == '.') {
        state->pos = start_pos;
        return lex_float(state, token);
    }

    token->type = TOK_NUMBERLIT;
    return ERR_NONE;
}

static dp_error_type lex_float(lexer_state *state, dp_token *token) {
    token->value.float_val = 0.0;
    while (is_digit(state->input[state->pos])) {
        token->value.float_val = token->value.float_val * 10 + (state->input[state->pos] - '0');
        lexer_advance(state);
    }

    // Lex the fractional part
    if (state->input[state->pos] == '.') {
        lexer_advance(state);
        double frac = 0.0;
        double div = 10.0;
        while (is_digit(state->input[state->pos])) {
            frac += (state->input[state->pos] - '0') / div;
            div *= 10.0;
            lexer_advance(state);
        }
        token->value.float_val += frac;
    }
    token->type = TOK_FLOATLIT;
    return ERR_NONE;
}

static dp_error_type lex_string(lexer_state *state, dp_token *token) {
    // Skip the opening double quote for the string literal
    lexer_advance(state);
    token->value.string = &state->input[state->pos];

    while (state->input[state->pos] != '"' && state->input[state->pos] != '\0') {
        // Consume as many characters as part of the string literal
        lexer_advance(state);
    }

    // Calculate the length of the string literal
    token->length = &state->input[state->pos] - token->value.string;
    if (state->input[state->pos] == '\0') {
        return ERR_UNTERMINATED_STRING;
    }

    token->type = TOK_STRINGLIT;
    // Skip the closing double quote for the string literal
    lexer_advance(state);
    return ERR_NONE;
}

static dp_error_type lex_char(lexer_state *state, dp_token *token) {
    int len = 0;

    // Skip the opening single quote for the character literal
    lexer_advance(state);
    if (state->input[state->pos] == '\0') {
        return ERR_UNTERMINATED_CHAR;
    }

    // Chars are numeric values in the lexer (stored as numbers)
    token->value.number = state->input[state->pos];
    len++;

    lexer_advance(state);

    if (state->input[state->pos] != '\'') {
        // Unterminated character literal (missing closing single quote)
        return ERR_UNTERMINATED_CHAR;
    }

    if (len > 1) {
        // Unterminated character literal (too many characters)
        return ERR_UNTERMINATED_CHAR;
    }

    token->type = TOK_CHARLIT;
    // Skip the closing single quote for the character literal
    lexer_advance(state);
    return ERR_NONE;
}

static dp_error_type lex_ident(lexer_state *state, dp_token *token) {
    token->value.ident = &state->input[state->pos];

    while (is_alpha(state->input[state->pos]) || is_digit(state->input[state->pos])) {
        // Consume as many characters as part of the identifier as long as they are
        // valid identifier characters (letters, digits, or underscore)
        lexer_advance(state);
    }

    token->length = &state->input[state->pos] - token->value.ident;
    token->type = TOK_IDENT;

    // Check if the identifier is a keyword
    for (size_t i = 0; i < NUM_KEYWORDS; i++) {
        size_t keyword_length = strlen(reserved_keywords[i].spelling);
        if (token->length == keyword_length &&
            memcmp(token->value.ident, reserved_keywords[i].spelling, keyword_length) == 0) {
            token->type = reserved_keywords[i].type;

            // Special handling for boolean literals
            if (token->type == TOK_BOOLLIT) {
                token->value.bool_val = token->value.ident[0] == 't';
            }
            break;
        }
    }

    return ERR_NONE;
}