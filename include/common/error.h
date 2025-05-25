#if defined(GRAMINA_NO_NAMESPACE) && !defined(Array)
#  define Optional(T) struct gramina_optional(T)
#endif
#if defined(GRAMINA_WANT_TAGLESS) && !defined(GraminaArray)
#  define GraminaOptional(T) struct gramina_optional(T)
#endif

#ifndef __GRAMINA_COMMON_ERROR_H
#define __GRAMINA_COMMON_ERROR_H

#include "def.h"
#include "str.h"

#define gramina_optional(T) gramina_optional

struct gramina_highlight_position {
    size_t start_line;
    size_t start_column;
    size_t end_line;
    size_t end_column;
};

struct gramina_highlight_info {
    struct gramina_string enable;
    struct gramina_string disable;
    struct gramina_string underline; 
};

struct gramina_string gramina_highlight_line(const struct gramina_string_view *source, struct gramina_highlight_position loc, const struct gramina_highlight_info *hi);

struct gramina_optional {
    int code;
    struct gramina_string message;
    void *value;
};

bool gramina_opt_has_value(const struct gramina_optional *this);
bool gramina_opt_has_error(const struct gramina_optional *this);

struct gramina_optional gramina_mk_opt_e(int code);
struct gramina_optional gramina_mk_opt_e_msg(int code, const struct gramina_string_view *msg);
struct gramina_optional gramina_mk_opt_e_msg_own(int code, struct gramina_string *msg);
struct gramina_optional gramina_mk_opt_e_msg_c(int code, const char *msg);

void gramina_opt_free(struct gramina_optional *this);

void *__gramina_get_opt_value_ptr(size_t val_size, bool prev); /* ignore */

#define gramina_mk_opt_v(T, v) ( \
    (struct gramina_optional) { \
        .code = 0, \
        .message = gramina_mk_str(), \
        .value = ( \
            *((T *)__gramina_get_opt_value_ptr(sizeof (v), false)) = (v), \
            __gramina_get_opt_value_ptr(sizeof (v), true) \
        ) \
    } \
)

// Make it point to a valid pointer to prevent NULL checks
// An optional of type `FILE` must never be created anyway
#define gramina_mk_opt_void() ( \
    (struct gramina_optional) { \
        .code = 0, \
        .message = gramina_mk_str(), \
        .value = stdin, \
    } \
)

#define gramina_no_value_expression(err) ( \
    fprintf( \
        stderr, \
        "at %s:%d\n  unwrapping valueless optional\n    code: %d\n    message: %.*s\n", \
        __FILE__, \
        __LINE__, \
        (err).code, \
        (int)(err).message.length, \
        (err).message.data \
    ), \
    gramina_trigger_debugger() \
)

#define gramina_opt_unwrap(T, opt) \
((opt).value == NULL \
    ? (gramina_no_value_expression(opt), *(T *)1) \
    : *((T *)((opt).value)))

#endif

#if defined(GRAMINA_NO_NAMESPACE) && !defined(__GRAMINA_OPTIONAL_NN_MACROS)
#  define __GRAMINA_OPTIONAL_NN_MACROS
#  define opt_unwrap gramina_opt_unwrap
#  define mk_opt_v gramina_mk_opt_v
#  define mk_opt_void gramina_mk_opt_void
#endif
#include "gen/common/error.h"
