#ifndef DOPPLER_ERROR_H
#define DOPPLER_ERROR_H

typedef enum {
    ERR_NONE = 0,
    ERR_UNEXPECTED_CHAR,
    ERR_UNTERMINATED_STRING,
    ERR_UNTERMINATED_CHAR,
    ERR_INVALID_NUMBER,
} dp_error_type;

typedef struct {
    const char *file;
    int line;
    int column;
} dp_error_location;

#ifndef FREESTANDING
#include <stdio.h>
#endif

inline void dp_error_print(dp_error_type err, dp_error_location loc) {
#ifndef FREESTANDING
    const char *err_str;
    switch (err) {
    case ERR_NONE:
        err_str = "No error";
        break;
    case ERR_UNEXPECTED_CHAR:
        err_str = "Unexpected character";
        break;
    case ERR_UNTERMINATED_STRING:
        err_str = "Unterminated string literal";
        break;
    case ERR_UNTERMINATED_CHAR:
        err_str = "Unterminated character literal";
        break;
    case ERR_INVALID_NUMBER:
        err_str = "Invalid number";
        break;
    default:
        err_str = "Unknown error";
        break;
    }
    printf("[ERROR] %s:%d:%d: %s\n", loc.file, loc.line, loc.column, err_str);
#endif
}

#endif // DOPPLER_ERROR_H