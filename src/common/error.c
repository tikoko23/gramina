#define GRAMINA_NO_NAMESPACE

#include "common/array.h"
#include "common/error.h"
#include "common/mem.h"

String gramina_highlight_line(const StringView *source, HighlightPosition loc, const HighlightInfo *hi) {
    size_t current_line = 1;
    size_t first_line_start = (size_t)-1;
    size_t last_line_start = (size_t)-1;
    size_t want_end = (size_t)-1;
    sv_foreach(i, ch, *source) {
        if (current_line == loc.start_line && first_line_start == (size_t)-1) {
            first_line_start = i;
        }

        if (current_line == loc.end_line && last_line_start == (size_t)-1) {
            last_line_start = i;
        }

        if (ch == '\n') {
            if (current_line == loc.end_line) {
                want_end = i;
                break;
            }

            ++current_line;
        }
    }

    if (first_line_start == (size_t)-1
     || last_line_start == (size_t)-1
     || want_end == (size_t)-1
     || first_line_start >= source->length
     || want_end >= source->length) {
        return mk_str_c("<error-while-generating-highlight>");
    }

    size_t padding = count_digits(loc.end_line);
    
    StringView wanted_region = sv_slice(source, first_line_start, want_end);

    String out = str_cfmt(" {cpf:%*zu} | ", (int)padding, loc.start_line);

    current_line = loc.start_line;
    size_t column = 1;
    sv_foreach(_, ch, wanted_region) {
        ++column;
        if (ch == '\n') {
            column = 1;
            ++current_line;

            str_cat_cfmt(&out, "\n {cpf:%*zu} | ", (int)padding, current_line);
        } else {
            str_append(&out, ch);
        }

        if (current_line == loc.end_line && column == loc.start_column) {
            str_cat(&out, &hi->enable);
        }

        if (current_line == loc.end_line && column == loc.end_column + 1) {
            str_cat(&out, &hi->disable);
        }
    }

    str_append(&out, '\n');

    for (size_t i = GRAMINA_ZERO; i < padding + 3 + loc.start_column; ++i) {
        str_append(&out, ' ');
    }

    str_cat(&out, &hi->underline);

    for (size_t i = GRAMINA_ZERO; i <= loc.end_column - loc.start_column; ++i) {
        str_append(&out, '~');
    }

    str_cat(&out, &hi->disable);

    return out;
}

typedef void * VoidPtr;

GRAMINA_DECLARE_ARRAY(VoidPtr);
GRAMINA_IMPLEMENT_ARRAY(VoidPtr);

bool gramina_opt_has_value(const Optional *this) {
    return this->value != NULL;
}

bool gramina_opt_has_error(const Optional *this) {
    return !opt_has_value(this);
}

Optional gramina_mk_opt_e(int code) {
    return (Optional) {
        .code = code,
        .message = mk_str(),
        .value = NULL,
    };
}

Optional gramina_mk_opt_e_msg(int code, const StringView *msg) {
    return (Optional) {
        .code = code,
        .message = sv_dup(msg),
        .value = NULL,
    };
}

Optional gramina_mk_opt_e_msg_own(int code, String *msg) {
    return (Optional) {
        .code = code,
        .message = *msg,
        .value = NULL,
    };
}

Optional gramina_mk_opt_e_msg_c(int code, const char *msg) {
    return (Optional) {
        .code = code,
        .message = mk_str_c(msg),
        .value = NULL,
    };
}

void *__gramina_get_opt_value_ptr(size_t val_size, bool prev) {
    static Array(VoidPtr) stack = {
        .items = NULL,
        .length = GRAMINA_ZERO,
        .capacity = GRAMINA_ZERO,
    };

    if (prev) {
        gramina_assert(stack.length > GRAMINA_ZERO, "no previous pointer\n");

        void *ptr = *array_last(VoidPtr, &stack);
        array_pop(VoidPtr, &stack);

        return ptr;
    }

    void *ptr = gramina_malloc(val_size);
    gramina_assert(ptr != NULL, "failed to allocate optional value of size %zu\n", val_size);

    array_append(VoidPtr, &stack, ptr);

    return ptr;
}

void gramina_opt_free(Optional *this) {
    if (this->value != NULL && this->value != stdin) {
        gramina_free(this->value);
        this->value = NULL;
    }

    str_free(&this->message);
}
