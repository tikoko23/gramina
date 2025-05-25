#define GRAMINA_NO_NAMESPACE

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common/def.h"
#include "common/mem.h"
#include "common/str.h"

static void __gramina_str_grow_at_least(String *this, size_t n) {
    if (this->capacity >= n) {
        return;
    }

    this->capacity = this->capacity * 3 / 2;
    if (this->capacity < n) {
        this->capacity = n;
    }

    this->data = gramina_realloc(this->data, this->capacity);
    if (this->data == NULL) {
        this->capacity = 0;
    }
}

String gramina_mk_str() {
    return (String) {
        .data = NULL,
        .length = 0,
        .capacity = 0,
    };
}

String gramina_mk_str_cap(size_t capacity) {
    String this = mk_str();
    str_reserve(&this, capacity);

    return this;
}

String gramina_mk_str_c(const char *cstr) {
    size_t len = strlen(cstr);
    String this = mk_str_cap(len);

    if (this.data == NULL) {
        return this;
    }

    this.length = len;

    memcpy(this.data, cstr, len);
    return this;
}

String gramina_mk_str_buf(const uint8_t *buf, size_t bufsize) {
    String this = mk_str_cap(bufsize);

    if (this.data == NULL) {
        return this;
    }

    this.length = bufsize;

    memcpy(this.data, buf, bufsize);
    return this;
}

StringView gramina_mk_sv_c(const char *cstr) {
    size_t len = strlen(cstr);
    return (StringView) {
        .data = cstr,
        .length = len,
    };
}

StringView gramina_mk_sv_buf(const uint8_t *buf, size_t bufsize) {
    return (StringView) {
        .data = (const char *)buf,
        .length = bufsize,
    };
}

String gramina_mk_str_sprintf(const StringView *_fmt, ...) {
    va_list args, args2;
    va_start(args, _fmt);
    va_copy(args2, args);

    String fmt = sv_dup(_fmt);
    str_append(&fmt, '\0');

    int n_bytes = vsnprintf(NULL, 0, fmt.data, args);

    va_end(args);

    if (n_bytes < 0) {
        str_free(&fmt);
        return mk_str();
    }

    String str = mk_str_cap(n_bytes + 1);
    n_bytes = vsnprintf(str.data, str.capacity, fmt.data, args2);

    str.length = n_bytes;
    if (str.data[n_bytes - 1] == '\0') {
        str_pop(&str);
    }

    va_end(args2);
    str_free(&fmt);

    return str;
}

String gramina_str_dup(const String *this) {
    String that = mk_str_cap(this->length);

    if (that.data == NULL) {
        return that;
    }

    that.length = this->length;
    memcpy(that.data, this->data, this->length);

    return that;
}

String gramina_sv_dup(const StringView *this) {
    String that = mk_str_buf((unsigned char *)this->data, this->length);
    return that;
}

StringView gramina_sv_slice(const StringView *this, size_t start, size_t end) {
    gramina_assert(start <= this->length, "start (%zu) index mustn't exceed length (%zu)\n", start, this->length);
    gramina_assert(end <= this->length, "end (%zu) index mustn't exceed length + 1 (%zu)\n", end, this->length);
    gramina_assert(start <= end, "start (%zu) index mustn't be greater than end (%zu)\n", start, end);

    return (StringView) {
        .data = this->data + start,
        .length = end - start
    };
}

StringView gramina_str_slice(const String *this, size_t start, size_t end) {
    StringView this_v = str_as_view(this);
    return sv_slice(&this_v, start, end);
}

StringView gramina_str_as_view(const String *this) {
    return (StringView) {
        .data = this->data,
        .length = this->length,
    };
}

void gramina_str_clear(struct gramina_string *this) {
    this->length = 0;
}

void gramina_str_shrink(String *this) {
    if (this->capacity == 0) {
        this->data = NULL;
    }

    this->data = gramina_realloc(this->data, this->length);

    gramina_assert(this->data != NULL, "alloc failed");

    if (this->data == NULL) {
        this->length = 0;
        this->capacity = 0;
    } else {
        this->capacity = this->length;
    }
}

void gramina_str_free(String *this) {
    gramina_free(this->data);

    this->data = NULL;
    this->length = 0;
    this->capacity = 0;
}

char *gramina_str_to_cstr(const String *this) {
    StringView sv = str_as_view(this);
    return gramina_sv_to_cstr(&sv);
}

char *gramina_sv_to_cstr(const StringView *this) {
    char *buf = gramina_malloc(this->length + 1);

    gramina_assert(buf != NULL, "alloc failed");

    if (buf == NULL) {
        return NULL;
    }

    memcpy(buf, this->data, this->length);
    buf[this->length] = '\0';

    return buf;
}

void gramina_str_reserve(String *this, size_t min_cap) {
    if (this->capacity >= min_cap) {
        return;
    }

    this->data = gramina_realloc(this->data, min_cap);

    gramina_assert(this->data != NULL, "alloc failed");

    if (this->data == NULL) {
        this->length = 0;
        this->capacity = 0;
    } else {
        this->capacity = min_cap;
    }
}

void gramina_str_pop(String *this) {
    if (this->length == 0) {
        return;
    }

    --this->length;
}

void gramina_str_append(String *this, char c) {
    str_reserve(this, this->length + 1);
    if (this->data == NULL) {
        return;
    }

    this->data[this->length++] = c;
}

void gramina_str_cat(String *this, const String *that) {
    StringView sv = str_as_view(that);
    gramina_str_cat_sv(this, &sv);
}

void gramina_str_cat_sv(String *this, const StringView *that) {
    str_reserve(this, this->length + that->length);
    if (this->data == NULL) {
        return;
    }

    char *copy_begin = this->data + this->length;
    memcpy(copy_begin, that->data, that->length);

    this->length += that->length;
}


void gramina_str_cat_cstr(String *this, const char *that) {
    StringView sv = mk_sv_c(that);
    str_cat_sv(this, &sv);
}

int gramina_str_cmp(const String *this, const String *that) {
    StringView this_v = str_as_view(this);
    StringView that_v = str_as_view(that);

    return sv_cmp(&this_v, &that_v);
}

int gramina_sv_cmp(const StringView *this, const StringView *that) {
    if (this->length == 0 && that->length == 0) {
        return 0;
    }

    for (size_t i = 0; i < this->length && i < that->length; ++i) {
        char a = this->data[i];
        char b = that->data[i];

        if (a != b) {
            return (unsigned char)a - (unsigned char)b;
        }
    }

    return (int)this->length - (int)that->length;
}

int gramina_str_cmp_c(const struct gramina_string *this, const char *that) {
    StringView this_v = str_as_view(this);
    StringView that_v = mk_sv_c(that);

    return sv_cmp(&this_v, &that_v);
}

int gramina_sv_cmp_c(const struct gramina_string_view *this, const char *that) {
    StringView that_v = mk_sv_c(that);

    return sv_cmp(this, &that_v);
}

String gramina_str_expand_tabs(const StringView *this, size_t tab_size) {
    String out = mk_str_cap(this->length);

    sv_foreach(_, ch, *this) {
        if (ch == '\t') {
            for (size_t i = 0; i < tab_size; ++i) {
                str_append(&out, ' ');
            }
        } else {
            str_append(&out, ch);
        }
    }

    return out;
}

static char get_hex_digit(uint8_t n) {
    if (n <= 9) {
        return '0' + n;
    }

    return 'A' + n - 10;
}

static void uint_to_hex(uint64_t n, size_t n_bytes, String *out) {
    for (int32_t i = n_bytes - 1; i >= 0; --i) {
        uint8_t exposed = (n >> i * 8) & 0xFF;

        str_append(out, get_hex_digit(exposed / 16));
        str_append(out, get_hex_digit(exposed % 16));
    }
}

static String format(const StringView *fmt, va_list args) {
    String out = mk_str();

    size_t bs_count = 0;
    size_t spec_start = (size_t)-1;
    size_t prec_start = (size_t)-1;
    sv_foreach(i, c, *fmt) {
        if (c == '{' && bs_count % 2 == 0) {
            spec_start = i + 1;
            continue;
        }

        if (c == ':' && spec_start != (size_t)-1) {
            prec_start = i + 1;
            continue;
        }

        if (c == '\\') {
            ++bs_count;
            continue;
        } else {
            for (size_t i = 0; i < bs_count && c != '{'; ++i) {
                str_append(&out, '\\');
            }

            bs_count = 0;
        }

        if (c == '}') {
            if (spec_start == (size_t)-1) {
                str_append(&out, '}');
                continue;
            }

            StringView spec = sv_slice(
                fmt,
                spec_start,
                prec_start == (size_t)-1
                    ? i
                    : prec_start - 1
            );

            StringView prec = prec_start == (size_t)-1
                            ? (StringView) { .data = NULL, .length = 0 }
                            : sv_slice(fmt, prec_start, i);

            if (sv_cmp_c(&spec, "c") == 0) {
                char arg = va_arg(args, int);

                str_append(&out, arg);

            } else if (sv_cmp_c(&spec, "s") == 0) {
                const String *arg = va_arg(args, const String *);

                str_cat(&out, arg);
            } else if (sv_cmp_c(&spec, "so") == 0) {
                String arg = va_arg(args, String);

                str_cat(&out, &arg);
                str_free(&arg);

            } else if (sv_cmp_c(&spec, "sv") == 0) {
                const StringView *arg = va_arg(args, const StringView *);

                str_cat_sv(&out, arg);

            } else if (sv_cmp_c(&spec, "svo") == 0) {
                StringView arg = va_arg(args, StringView);

                str_cat_sv(&out, &arg);

            } else if (sv_cmp_c(&spec, "cstr") == 0) {
                const char *arg = va_arg(args, const char *);
                StringView view = mk_sv_c(arg);

                str_cat_sv(&out, &view);

            } else if (sv_cmp_c(&spec, "i32") == 0) {
                int32_t arg = va_arg(args, int32_t);
                String str = i32_to_str(arg);

                str_cat(&out, &str);
                str_free(&str);

            } else if (sv_cmp_c(&spec, "u32") == 0) {
                uint32_t arg = va_arg(args, uint32_t);
                String str = u32_to_str(arg);

                str_cat(&out, &str);
                str_free(&str);

            } else if (sv_cmp_c(&spec, "i64") == 0) {
                int64_t arg = va_arg(args, int32_t);
                String str = i64_to_str(arg);

                str_cat(&out, &str);
                str_free(&str);

            } else if (sv_cmp_c(&spec, "u64") == 0) {
                uint64_t arg = va_arg(args, uint32_t);
                String str = u64_to_str(arg);

                str_cat(&out, &str);
                str_free(&str);

            } else if (sv_cmp_c(&spec, "x32") == 0) {
                uint32_t arg = va_arg(args, uint32_t);
                String str = mk_str_cap(8);

                uint_to_hex(arg, 4, &str);

                str_append(&out, '0');
                str_append(&out, 'x');
                str_cat(&out, &str);
                str_free(&str);

            } else if (sv_cmp_c(&spec, "x64") == 0) {
                uint64_t arg = va_arg(args, uint64_t);
                String str = mk_str_cap(16);

                uint_to_hex(arg, 8, &str);

                str_append(&out, '0');
                str_append(&out, 'x');
                str_cat(&out, &str);
                str_free(&str);

            } else if (sv_cmp_c(&spec, "f32") == 0) {
                float arg = va_arg(args, double);
                String str = prec.length > 0
                           ? f32_prec_to_str(arg, sv_to_u64(&prec))
                           : f32_to_str(arg);

                str_cat(&out, &str);
                str_free(&str);

            } else if (sv_cmp_c(&spec, "f64") == 0) {
                double arg = va_arg(args, double);
                String str = prec.length > 0
                           ? f64_prec_to_str(arg, sv_to_u64(&prec))
                           : f64_to_str(arg);

                str_cat(&out, &str);
                str_free(&str);
            } else if (sv_cmp_c(&spec, "cpf") == 0) {
                char *cfmt = sv_to_cstr(&prec);
                char buf[1024];
                int written = vsnprintf(buf, (sizeof buf) - 1, cfmt, args);
                if (written < 0) {
                    gramina_free(cfmt);
                    str_free(&out);
                    return mk_str();
                }

                StringView sv = mk_sv_c(buf);
                str_cat_sv(&out, &sv);
                gramina_free(cfmt);

            } else {
                str_free(&out);
                return mk_str();
            }

            spec_start = (size_t)-1;
            prec_start = (size_t)-1;
            continue;
        }

        if (spec_start == (size_t)-1) {
            str_append(&out, c);
        }
    }

    return out;
}

String gramina_str_cfmt(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    String ret = str_vcfmt(fmt, args);

    va_end(args);
    return ret;
}

String gramina_str_fmt(const StringView *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    String ret = str_vfmt(fmt, args);

    va_end(args);
    return ret;
}

String gramina_str_vcfmt(const char *fmt, va_list args) {
    StringView view = mk_sv_c(fmt);
    return format(&view, args);
}

String gramina_str_vfmt(const StringView *fmt, va_list args) {
    return format(fmt, args);
}

void gramina_str_cat_cfmt(String *restrict this, const char *restrict fmt, ...) {
    va_list args;
    va_start(args, fmt);

    String ret = str_vcfmt(fmt, args);

    va_end(args);

    str_cat(this, &ret);
    str_free(&ret);
}

void gramina_str_cat_fmt(String *restrict this, const StringView *restrict fmt, ...) {
    va_list args;
    va_start(args, fmt);

    String ret = str_vfmt(fmt, args);

    va_end(args);

    str_cat(this, &ret);
    str_free(&ret);
}

void gramina_str_cat_vcfmt(String *restrict this, const char *restrict fmt, va_list args) {
    String ret = str_vcfmt(fmt, args);

    str_cat(this, &ret);
    str_free(&ret);
}

void gramina_str_cat_vfmt(String *restrict this, const StringView *restrict fmt, va_list args) {
    String ret = str_vfmt(fmt, args);

    str_cat(this, &ret);
    str_free(&ret);
}

size_t gramina_count_digits(uint64_t x) {
    size_t n = 0;

    do {
        x /= 10;
        ++n;
    } while(x);

    return n;
}

String gramina_i32_to_str(int32_t n) {
    return i64_to_str(n);
}

String gramina_u32_to_str(uint32_t n) {
    return u64_to_str(n);
}

String gramina_i64_to_str(int64_t n) {
    String this = n < 0
                ? mk_str_c("-")
                : mk_str();

    if (n < 0) {
        n *= -1;
    }

    String raw = u64_to_str(n);
    str_cat(&this, &raw);

    str_free(&raw);

    return this;
}

String gramina_u64_to_str(uint64_t n) {
    size_t n_digits = count_digits(n);
    String this = mk_str_cap(n_digits);
    this.length = n_digits;

    --n_digits;

    do {
        uint64_t digit = n % 10;
        n /= 10;

        this.data[n_digits] = digit + '0';
        --n_digits;
    } while (n);

    return this;
}

String gramina_f32_to_str(float x) {
    return f32_prec_to_str(x, 6);
}

String gramina_f64_to_str(double x) {
    return f64_prec_to_str(x, 6);
}

String gramina_f32_prec_to_str(float x, int prec) {
    return f64_prec_to_str(x, prec);
}

String gramina_f64_prec_to_str(double x, int prec) {
    char buf[64];
    int written = snprintf(buf, (sizeof buf), "%.*lf", prec, x);
    if (written < 0) {
        return mk_str();
    }

    buf[written] = '\0';
    return mk_str_c(buf);
}

int64_t gramina_sv_to_i64(const StringView *this) {
    if (this->length == 0) {
        return 0;
    }

    bool negative = this->data[0] == '-';

    StringView view = negative
                    ? sv_slice(this, 1, this->length)
                    : *this;

    return (int64_t)sv_to_u64(&view) * (negative ? -1 : 1);
}

uint64_t gramina_sv_to_u64(const StringView *this) {
    uint64_t n = 0;
    sv_foreach(_, c, *this) {
        if (!isdigit(c)) {
            break;
        }

        n *= 10;
        n += c - '0';
    }

    return n;
}

int gramina_print_cfmt(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String out = str_vcfmt(fmt, args);
    va_end(args);

    int written = fwrite(out.data, 1, out.length, stdout);
    str_free(&out);

    return written;
}
