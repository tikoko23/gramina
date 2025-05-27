#include "def.h"

#ifndef gramina_hashmap_foreach
#  define gramina_hashmap_foreach(T, _key, _value, hashmap) \
   GRAMINA_BEGIN_BLOCK_DECL() \
   GRAMINA_BLOCK_DECL(struct gramina_string_view _key) \
   GRAMINA_BLOCK_DECL(T *_value) \
   GRAMINA_BLOCK_DECL(struct gramina_hashmap __hashmap = (hashmap)) \
   for (size_t __bi = 0; __bi < __hashmap.n_buckets; ++__bi) \
   for (size_t __ii = 0; __ii < __hashmap.buckets[__bi].length \
        && (_key = gramina_str_as_view(&__hashmap.buckets[__bi].items[__ii].key), \
            _value = __hashmap.buckets[__bi].items[__ii].value); \
        ++__ii) \

#endif
#if !defined(hashmap_foreach) && defined(GRAMINA_NO_NAMESPACE)
#  define hashmap_foreach(...) gramina_hashmap_foreach(__VA_ARGS__)
#endif

#ifndef __GRAMINA_COMMON_HASHMAP_H
#define __GRAMINA_COMMON_HASHMAP_H

#include "array.h"
#include "str.h"

typedef struct {
    struct gramina_string key;
    void *value;
} HashmapItem;

GRAMINA_DECLARE_ARRAY(HashmapItem)

struct gramina_hashmap {
    size_t n_buckets;
    struct gramina_array(HashmapItem) *buckets; // Array of arrays, not pointer to Array
    void (*object_freer)(void *);
};

struct gramina_hashmap gramina_mk_hashmap(size_t n_buckets);
struct gramina_hashmap gramina_hashmap_dup(const struct gramina_hashmap *this);

size_t gramina_hashmap_count(const struct gramina_hashmap *this);

void gramina_hashmap_set(struct gramina_hashmap *this, struct gramina_string_view key, void *value);
void *gramina_hashmap_get(const struct gramina_hashmap *this, struct gramina_string_view key);
void gramina_hashmap_remove(struct gramina_hashmap *this, struct gramina_string_view key);

void gramina_hashmap_set_c(struct gramina_hashmap *this, const char *key, void *value);
void *gramina_hashmap_get_c(const struct gramina_hashmap *this, const char *key);
void gramina_hashmap_remove_c(struct gramina_hashmap *this, const char *key);

void gramina_hashmap_free(struct gramina_hashmap *this);

#endif
#include "gen/common/hashmap.h"
