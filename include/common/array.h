/* gen_ignore: true */

#if defined(GRAMINA_NO_NAMESPACE) && !defined(Array)
#  define array_foreach(...) gramina_array_foreach(__VA_ARGS__)
#  define array_foreach_ref(...) gramina_array_foreach_ref(__VA_ARGS__)
#  define slice_foreach(...) gramina_slice_foreach(__VA_ARGS__)
#  define slice_foreach_ref(...) gramina_slice_foreach_ref(__VA_ARGS__)

#  define Array(T) struct gramina_array(T)
#  define Slice(T) struct gramina_slice(T)
#  define GRAMINA_ARRAY_TAGLESS_TYPEDEFS(T)
#  define GRAMINA_ARRAY_NN_TYPEDEFS(T) \
   typedef struct gramina_ ## T ## _array T ## Array; \
   typedef struct gramina_ ## T ## _slice T ## Slice;
#endif
#if defined(GRAMINA_WANT_TAGLESS) && !defined(GraminaArray)
#  define GraminaArray(T) struct gramina_array(T)
#  define GraminaSlice(T) struct gramina_slice(T)
#  undef GRAMINA_ARRAY_TAGLESS_TYPEDEFS
#  define GRAMINA_ARRAY_TAGLESS_TYPEDEFS(T) \
   typedef struct gramina_ ## T ## _array Gramina ## T ## Array; \
   typedef struct gramina_ ## T ## _slice Gramina ## T ## Slice;
#  ifndef GRAMINA_ARRAY_NN_TYPEDEFS
#    define GRAMINA_ARRAY_NN_TYPEDEFS(T)
#  endif
#endif

#ifndef __GRAMINA_COMMON_ARRAY_H
#define __GRAMINA_COMMON_ARRAY_H

#include <stddef.h>

#include "def.h"
#include "mem.h"

#define gramina_array(T) gramina_ ## T ## _array
#define gramina_slice(T) gramina_ ## T ## _slice

#if !defined(GRAMINA_NO_NAMESPACE) && !defined(GRAMINA_WANT_TAGLESS)
#  define GRAMINA_ARRAY_TAGLESS_TYPEDEFS(T)
#  define GRAMINA_ARRAY_NN_TYPEDEFS(T)
#endif

#define gramina_array_foreach(T, index_var, value_var, arr) \
GRAMINA_BEGIN_BLOCK_DECL() \
GRAMINA_BLOCK_DECL(size_t index_var = 0) \
GRAMINA_BLOCK_DECL(T value_var) \
for (struct gramina_array(T) __arr = (arr); index_var < __arr.length && (value_var = __arr.items[index_var], 1); ++index_var)

#define gramina_array_foreach_ref(T, index_var, value_var, arr) \
GRAMINA_BEGIN_BLOCK_DECL() \
GRAMINA_BLOCK_DECL(size_t index_var = 0) \
GRAMINA_BLOCK_DECL(T *value_var) \
for (struct gramina_array(T) __arr = (arr); index_var < __arr.length && (value_var = &__arr.items[index_var], 1); ++index_var)

#define gramina_slice_foreach(T, index_var, value_var, arr) \
GRAMINA_BEGIN_BLOCK_DECL() \
GRAMINA_BLOCK_DECL(size_t index_var = 0) \
GRAMINA_BLOCK_DECL(T value_var) \
for (struct gramina_slice(T) __arr = (arr); index_var < __arr.length && (value_var = __arr.items[index_var], 1); ++index_var)

#define gramina_slice_foreach_ref(T, index_var, value_var, arr) \
GRAMINA_BEGIN_BLOCK_DECL() \
GRAMINA_BLOCK_DECL(size_t index_var = 0) \
GRAMINA_BLOCK_DECL(const T *value_var) \
for (struct gramina_slice(T) __arr = (arr); index_var < __arr.length && (value_var = &__arr.items[index_var], 1); ++index_var)

#ifndef GRAMINA_ARRAY_GROW_EXP
#  define GRAMINA_ARRAY_GROW_EXP(cap) ((cap) * 3 / 2)
#endif

#define GRAMINA_DECLARE_ARRAY(T, ...) \
struct gramina_ ## T ## _array { \
    size_t length; \
    size_t capacity; \
    T *items; \
}; \
struct gramina_ ## T ## _slice { \
    size_t length; \
    const T *items; \
}; \
GRAMINA_ARRAY_NN_TYPEDEFS(T) \
GRAMINA_ARRAY_TAGLESS_TYPEDEFS(T) \
__VA_ARGS__ struct gramina_array(T) gramina_mk_ ## T ## _array(); \
__VA_ARGS__ struct gramina_array(T) gramina_mk_ ## T ## _array_capacity(size_t cap); \
__VA_ARGS__ struct gramina_array(T) gramina_mk_ ## T ## _array_prefilled(size_t n, T v); \
__VA_ARGS__ struct gramina_array(T) gramina_mk_ ## T ## _array_c(const T *carray, size_t n); \
__VA_ARGS__ struct gramina_array(T) gramina_ ## T ## _array_dup(const struct gramina_array(T) *this); \
\
__VA_ARGS__ struct gramina_slice(T) gramina_ ## T ## _array_slice(const struct gramina_array(T) *this, size_t start, size_t end); \
__VA_ARGS__ struct gramina_slice(T) gramina_ ## T ## _array_as_slice(const struct gramina_array(T) *this); \
__VA_ARGS__ struct gramina_slice(T) gramina_ ## T ## _slice(const struct gramina_slice(T) *this, size_t start, size_t end); \
\
__VA_ARGS__ void gramina_ ## T ## _array_shrink(struct gramina_array(T) *this); \
__VA_ARGS__ void gramina_ ## T ## _array_free(struct gramina_array(T) *this); \
__VA_ARGS__ void gramina_ ## T ## _array_reserve(struct gramina_array(T) *this, size_t min_cap); \
__VA_ARGS__ void gramina_ ## T ## _array_append(struct gramina_array(T) *this, T v); \
__VA_ARGS__ void gramina_ ## T ## _array_pop(struct gramina_array(T) *this); \
__VA_ARGS__ void gramina_ ## T ## _array_remove(struct gramina_array(T) *this, size_t index); \
__VA_ARGS__ T *gramina_ ## T ## _array_first(const struct gramina_array(T) *this); \
__VA_ARGS__ T *gramina_ ## T ## _array_last(const struct gramina_array(T) *this); \

#define GRAMINA_IMPLEMENT_ARRAY(T, ...) \
static void __gramina_ ## T ## _array_grow_at_least(struct gramina_array(T) *this, size_t n) { \
    if (this->capacity >= n) { \
        return; \
    } \
    \
    this->capacity = GRAMINA_ARRAY_GROW_EXP(this->capacity); \
    if (this->capacity < n) { \
        this->capacity = n; \
    } \
    \
    this->items = gramina_realloc(this->items, this->capacity * sizeof (T)); \
    if (this->items == NULL) { \
        this->length = 0; \
        this->capacity = 0; \
    } \
} \
\
__VA_ARGS__ struct gramina_array(T) gramina_mk_ ## T ## _array() { \
    return (struct gramina_array(T)) { \
        .items = NULL, \
        .length = 0, \
        .capacity = 0, \
    }; \
} \
\
__VA_ARGS__ struct gramina_array(T) gramina_mk_ ## T ## _array_capacity(size_t cap) { \
    struct gramina_array(T) this = gramina_mk_ ## T ## _array(); \
    if (cap == 0) { \
        return this; \
    } \
    \
    gramina_ ## T ## _array_reserve(&this, cap); \
    if (this.items != NULL) { \
        this.capacity = cap; \
    } \
    \
    return this; \
} \
\
__VA_ARGS__ struct gramina_array(T) gramina_mk_ ## T ## _array_prefilled(size_t n, T v) { \
    struct gramina_array(T) this = gramina_mk_ ## T ## _array_capacity(n); \
    if (this.items == NULL) { \
        return this; \
    } \
    \
    this.length = n; \
    for (size_t i = 0; i < n; ++i) { \
        this.items[i] = v; \
    } \
    \
    return this; \
} \
\
__VA_ARGS__ struct gramina_array(T) gramina_mk_ ## T ## _array_c(const T *carray, size_t n) { \
    struct gramina_array(T) this = gramina_mk_ ## T ## _array_capacity(n); \
    \
    for (size_t i = 0; i < n; ++i) { \
        this.items[i] = carray[i]; \
    } \
    \
    this.length = n; \
    \
    return this; \
} \
\
__VA_ARGS__ struct gramina_array(T) gramina_ ## T ## _array_dup(const struct gramina_array(T) *this) { \
    struct gramina_array(T) that = gramina_mk_ ## T ## _array_capacity(this->capacity); \
    gramina_array_foreach_ref(T, i, v, *this) { \
        that.items[i] = *v; \
    } \
    \
    return that; \
} \
\
__VA_ARGS__ struct gramina_slice(T) gramina_ ## T ## _array_slice(const struct gramina_array(T) *this, size_t start, size_t end) { \
    gramina_assert(start <= this->length, "start (%zu) index mustn't exceed length (%zu)\n", start, this->length); \
    gramina_assert(end <= this->length, "end (%zu) index mustn't exceed length (%zu)\n", end, this->length); \
    gramina_assert(start <= end, "start (%zu) index mustn't be greater than end (%zu)\n", start, end); \
    return (struct gramina_slice(T)) { \
        .items = this->items + start, \
        .length = end - start, \
    }; \
} \
\
__VA_ARGS__ struct gramina_slice(T) gramina_ ## T ## _array_as_slice(const struct gramina_array(T) *this) { \
    return gramina_ ## T ## _array_slice(this, 0, this->length); \
} \
\
__VA_ARGS__ struct gramina_slice(T) gramina_ ## T ## _slice(const struct gramina_slice(T) *this, size_t start, size_t end) { \
    gramina_assert(start <= this->length, "start (%zu) index mustn't exceed length (%zu)\n", start, this->length); \
    gramina_assert(end <= this->length, "end (%zu) index mustn't exceed length (%zu)\n", end, this->length); \
    gramina_assert(start <= end, "start (%zu) index mustn't be greater than end (%zu)\n", start, end); \
    return (struct gramina_slice(T)) { \
        .items = this->items + start, \
        .length = end - start, \
    }; \
} \
\
__VA_ARGS__ void gramina_ ## T ## _array_shrink(struct gramina_array(T) *this) { \
    if (this->capacity == 0) { \
        this->items = NULL; \
    } \
    \
    this->items = gramina_realloc(this->items, this->length * sizeof (T)); \
    if (this->items == NULL) { \
        this->length = 0; \
        this->capacity = 0; \
    } else { \
        this->capacity = this->length; \
    } \
} \
\
__VA_ARGS__ void gramina_ ## T ## _array_free(struct gramina_array(T) *this) { \
    if (this->items != NULL) { \
        gramina_free(this->items); \
    } \
    \
    this->items = NULL; \
    this->length = 0; \
    this->capacity = 0; \
} \
\
__VA_ARGS__ void gramina_ ## T ## _array_reserve(struct gramina_array(T) *this, size_t min_cap) { \
    if (this->capacity >= min_cap) { \
        return; \
    } \
    \
    this->capacity = min_cap; \
    this->items = gramina_realloc(this->items, this->capacity * sizeof (T)); \
    if (this->items == NULL) { \
        this->length = 0; \
        this->capacity = 0; \
    } \
} \
\
__VA_ARGS__ void gramina_ ## T ## _array_append(struct gramina_array(T) *this, T v) { \
    __gramina_ ## T ## _array_grow_at_least(this, this->length + 1); \
    this->items[this->length++] = v; \
} \
\
__VA_ARGS__ void gramina_ ## T ## _array_pop(struct gramina_array(T) *this) { \
    if (this->length != 0) { \
        --this->length; \
    } \
} \
\
__VA_ARGS__ void gramina_ ## T ## _array_remove(struct gramina_array(T) *this, size_t index) { \
    if (index >= this->length) { \
        return; \
    } \
    \
    for (size_t i = index; i < this->length - 1; ++i) { \
        this->items[i] = this->items[i + 1]; \
    } \
    \
    gramina_ ## T ## _array_pop(this); \
} \
\
__VA_ARGS__ T *gramina_ ## T ## _array_first(const struct gramina_array(T) *this) { \
    return &this->items[0]; \
} \
\
__VA_ARGS__ T *gramina_ ## T ## _array_last(const struct gramina_array(T) *this) { \
    if (this->length == 0) { \
        return NULL; \
    } \
    \
    return &this->items[this->length - 1]; \
} \
\

#endif

#if defined(GRAMINA_NO_NAMESPACE) && !defined(__GRAMINA_ARRAY_NN_MACROS)
#  define __GRAMINA_ARRAY_NN_MACROS
#  define mk_array(T) gramina_mk_ ## T ## _array()
#  define mk_array_capacity(T, cap) gramina_mk_ ## T ## _array_capacity(cap)
#  define mk_array_prefilled(T, n, v) gramina_mk_ ## T ## _array_prefilled(n, v)
#  define mk_array_c(T, carray, n) gramina_mk_ ## T ## _array_c(carray, n)
#  define array_dup(T, this) gramina_ ## T ## _array_dup(this)
#  define slice(T, this, start, end) gramina_ ## T ## _slice(this, start, end)
#  define array_slice(T, this, start, end) gramina_ ## T ## _array_slice(this, start, end)
#  define array_as_slice(T, this) gramina_ ## T ## _array_as_slice(this)
#  define array_shrink(T, this) gramina_ ## T ## _array_free(this)
#  define array_free(T, this) gramina_ ## T ## _array_free(this)
#  define array_reserve(T, this, min_cap) gramina_ ## T ## _array_reserve(this, min_cap)
#  define array_append(T, this, v) gramina_ ## T ## _array_append(this, v)
#  define array_pop(T, this) gramina_ ## T ## _array_pop(this)
#  define array_remove(T, this, idx) gramina_ ## T ## _array_remove(this, idx)
#  define array_first(T, this) gramina_ ## T ## _array_first(this)
#  define array_last(T, this) gramina_ ## T ## _array_last(this)
#endif
