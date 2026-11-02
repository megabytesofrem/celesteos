#include "lexer.h"

void lexer_init(lexer_state *state, const char *input) {
    state->input = input;
    state->pos = 0;
    state->line = 1;
    state->column = 1;
}

static void skip_ws(lexer_state *state);

static dp_error_type lex_number(lexer_state *state, dp_token *token);
static dp_error_type lex_float(lexer_state *state, dp_token *token);
static dp_error_type lex_string(lexer_state *state, dp_token *token);

dp_error_type lexer_next(lexer_state *state, dp_token *token) {
    skip_ws(state);

    token->line = state->line;
    token->column = state->column;
    token->length = 0;

    switch (state->input[state->pos]) {
    case '\0':
        token->type = TOK_EOF;
        return ERR_NONE;
    case '"':
        return lex_string(state, token);
    case '0' ... '9':
        return lex_number(state, token);
    // Add cases for other token types here.
    default:
        state->pos++;
        state->column++;
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
    while (state->input[state->pos] >= '0' && state->input[state->pos] <= '9') {
        token->value.number = token->value.number * 10 + (state->input[state->pos] - '0');
        lexer_advance(state);
    }

    if (state->input[state->pos] == '.') {
        state->pos = start_pos;
        return lex_float(state, token);
    }

    token->type = TOK_NUMBER;
    return 0;
}

static dp_error_type lex_float(lexer_state *state, dp_token *token) {
    token->value.float_val = 0.0;
    while (state->input[state->pos] >= '0' && state->input[state->pos] <= '9') {
        token->value.float_val = token->value.float_val * 10 + (state->input[state->pos] - '0');
        lexer_advance(state);
    }
    if (state->input[state->pos] == '.') {
        lexer_advance(state);
        double frac = 0.0;
        double div = 10.0;
        while (state->input[state->pos] >= '0' && state->input[state->pos] <= '9') {
            frac += (state->input[state->pos] - '0') / div;
            div *= 10.0;
            lexer_advance(state);
        }
        token->value.float_val += frac;
    }
    token->type = TOK_FLOAT;
    return 0;
}

static dp_error_type lex_string(lexer_state *state, dp_token *token) {
    lexer_advance(state);
    token->value.string = &state->input[state->pos];

    while (state->input[state->pos] != '"' && state->input[state->pos] != '\0') {
        lexer_advance(state);
    }

    token->length = &state->input[state->pos] - token->value.string;
    if (state->input[state->pos] == '\0') {
        return ERR_UNTERMINATED_STRING;
    }

    token->type = TOK_STRING;
    lexer_advance(state);
    return 0;
}