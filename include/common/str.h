#ifndef __GRAMINA_COMMON_STR_H
#define __GRAMINA_COMMON_STR_H

#include <stddef.h>
#include <stdint.h>

#include "def.h"

#define __gramina_str_foreach_guts(index_var, ch_var, str_decl) \
GRAMINA_BEGIN_BLOCK_DECL() \
GRAMINA_BLOCK_DECL(char ch_var = '\0') \
GRAMINA_BLOCK_DECL(size_t index_var = 0) \
for (str_decl; index_var < __str.length && (ch_var = __str.data[index_var], 1); ++index_var)

#define gramina_str_foreach(index_var, ch_var, str) \
__gramina_str_foreach_guts(index_var, ch_var, String __str = (str))

#define gramina_str_foreach_ref(index_var, ch_var, str) \
GRAMINA_BEGIN_BLOCK_DECL() \
GRAMINA_BLOCK_DECL(char *ch_var = NULL) \
GRAMINA_BLOCK_DECL(size_t index_var = 0) \
for (String __str = (str); index_var < __str.length && (ch_var = &__str.data[index_var], 1); ++index_var)

#define gramina_sv_foreach(index_var, ch_var, sv) \
__gramina_str_foreach_guts(index_var, ch_var, StringView __str = (sv))

#ifdef GRAMINA_NO_NAMESPACE
#  define str_foreach gramina_str_foreach
#  define str_foreach_ref gramina_str_foreach_ref
#  define sv_foreach gramina_sv_foreach
#endif

struct gramina_string {
    size_t length;
    size_t capacity;
    char *data;
};

struct gramina_string_view {
    size_t length;
    const char *data;
};

struct gramina_string gramina_mk_str();
struct gramina_string gramina_mk_str_cap(size_t capacity);
struct gramina_string gramina_mk_str_c(const char *cstr);
struct gramina_string gramina_mk_str_buf(const uint8_t *buf, size_t bufsize);
struct gramina_string gramina_mk_str_sprintf(const struct gramina_string_view *fmt, ...);
struct gramina_string gramina_mk_str_csprintf(const char *fmt, ...);

struct gramina_string_view gramina_mk_sv_c(const char *cstr);
struct gramina_string_view gramina_mk_sv_buf(const uint8_t *buf, size_t bufsize);

struct gramina_string gramina_str_dup(const struct gramina_string *this);
struct gramina_string gramina_sv_dup(const struct gramina_string_view *this);

struct gramina_string_view gramina_sv_slice(const struct gramina_string_view *this, size_t start, size_t end);
struct gramina_string_view gramina_str_slice(const struct gramina_string *this, size_t start, size_t end);
struct gramina_string_view gramina_str_as_view(const struct gramina_string *this);

void gramina_str_clear(struct gramina_string *this);
// Allocates as little memory as possible for `this`
void gramina_str_shrink(struct gramina_string *this);
void gramina_str_free(struct gramina_string *this);

// *!* MUST BE FREED WITH `gramina_free` *!*
char *gramina_str_to_cstr(const struct gramina_string *this);
// *!* MUST BE FREED WITH `gramina_free` *!*
char *gramina_sv_to_cstr(const struct gramina_string_view *this);

void gramina_str_reserve(struct gramina_string *this, size_t min_cap);

void gramina_str_pop(struct gramina_string *this);
void gramina_str_append(struct gramina_string *this, char c);

void gramina_str_cat(struct gramina_string *this, const struct gramina_string *that);
void gramina_str_cat_sv(struct gramina_string *this, const struct gramina_string_view *that);
void gramina_str_cat_cstr(struct gramina_string *this, const char *that);

int gramina_str_cmp(const struct gramina_string *this, const struct gramina_string *that);
int gramina_sv_cmp(const struct gramina_string_view *this, const struct gramina_string_view *that);
int gramina_str_cmp_c(const struct gramina_string *this, const char *that);
int gramina_sv_cmp_c(const struct gramina_string_view *this, const char *that);

struct gramina_string gramina_str_cfmt(const char *restrict fmt, ...);
struct gramina_string gramina_str_fmt(const struct gramina_string_view *restrict fmt, ...);
struct gramina_string gramina_str_vcfmt(const char *restrict fmt, va_list args);
struct gramina_string gramina_str_vfmt(const struct gramina_string_view *restrict fmt, va_list args);

void gramina_str_cat_cfmt(struct gramina_string *restrict this, const char *restrict fmt, ...);
void gramina_str_cat_fmt(struct gramina_string *restrict this, const struct gramina_string_view *restrict fmt, ...);
void gramina_str_cat_vcfmt(struct gramina_string *restrict this, const char *restrict fmt, va_list args);
void gramina_str_cat_vfmt(struct gramina_string *restrict this, const struct gramina_string_view *restrict fmt, va_list args);

struct gramina_string gramina_str_expand_tabs(const struct gramina_string_view *this, size_t tab_size);

struct gramina_string gramina_i32_to_str(int32_t n);
struct gramina_string gramina_u32_to_str(uint32_t n);

struct gramina_string gramina_i64_to_str(int64_t n);
struct gramina_string gramina_u64_to_str(uint64_t n);

struct gramina_string gramina_f32_to_str(float x);
struct gramina_string gramina_f64_to_str(double x);
struct gramina_string gramina_f32_prec_to_str(float x, int prec);
struct gramina_string gramina_f64_prec_to_str(double x, int prec);

int64_t gramina_sv_to_i64(const struct gramina_string_view *this);
uint64_t gramina_sv_to_u64(const struct gramina_string_view *this);

size_t gramina_count_digits(uint64_t x);

int gramina_print_cfmt(const char *fmt, ...);

#endif
#include "gen/common/str.h"
