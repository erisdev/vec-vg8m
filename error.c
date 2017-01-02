#include "internal.h"
#include <stdarg.h>

static char _error_message[512];

void origin_set_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(_error_message, sizeof(_error_message), fmt, args);
    va_end(args);
}

const char *origin_error() {
    return _error_message;
}
