#define GRAMINA_NO_NAMESPACE

#include <stdint.h>

#include "common/hashmap.h"

GRAMINA_IMPLEMENT_ARRAY(HashmapItem)

// FNV-1a
static uint64_t hash_str(const StringView *str) {
    uint64_t h = 0xCBF29CE484222325;

    sv_foreach(_, b, *str) {
        h ^= b;
        h *= 0x100000001B3;
    }

    return h;
}

static size_t get_bucket(const Hashmap *this, const StringView *str) {
    return hash_str(str) % this->n_buckets;
}

Hashmap gramina_mk_hashmap(size_t n_buckets) {
    if (n_buckets == 0) {
        return (Hashmap) {
            .n_buckets = 0,
            .buckets = NULL,
            .object_freer = NULL,
        };
    }

    Hashmap this = {
        .n_buckets = n_buckets
    };

    this.buckets = gramina_malloc((sizeof *this.buckets) * n_buckets);
    if (!this.buckets) {
        return (Hashmap) {
            .n_buckets = 0,
            .buckets = NULL,
            .object_freer = NULL,
        };
    }

    for (size_t i = 0; i < n_buckets; ++i) {
        this.buckets[i] = mk_array(HashmapItem);
    }

    this.object_freer = NULL;
    return this;
}

Hashmap gramina_hashmap_dup(const Hashmap *this) {
    Hashmap that = mk_hashmap(this->n_buckets);
    that.object_freer = this->object_freer;

    that.buckets = gramina_malloc((sizeof *that.buckets) * that.n_buckets);
    if (!that.buckets) {
        return (Hashmap) {
            .n_buckets = 0,
            .buckets = NULL,
            .object_freer = NULL,
        };
    }

    for (size_t i = 0; i < that.n_buckets; ++i) {
        that.buckets[i] = array_dup(HashmapItem, &this->buckets[i]);
    }

    return that;
}

void gramina_hashmap_set(Hashmap *this, StringView key, void *value) {
    size_t bucket = get_bucket(this, &key);
    
    array_foreach_ref(HashmapItem, _, item, this->buckets[bucket]) {
        StringView this_key = str_as_view(&item->key);
        if (sv_cmp(&this_key, &key) == 0) {
            if (this->object_freer) {
                this->object_freer(item->value);
            }

            str_free(&item->key);

            item->value = value;
            return;
        }
    }

    HashmapItem item = {
        .key = sv_dup(&key),
        .value = value,
    };

    array_append(HashmapItem, &this->buckets[bucket], item);
}

void *gramina_hashmap_get(const Hashmap *this, StringView key) {
    size_t bucket = get_bucket(this, &key);
    
    array_foreach_ref(HashmapItem, _, item, this->buckets[bucket]) {
        StringView this_key = str_as_view(&item->key);
        if (sv_cmp(&this_key, &key) == 0) {
            return item->value;
        }
    }

    return NULL;
}

void gramina_hashmap_remove(Hashmap *this, StringView key) {
    size_t bucket = get_bucket(this, &key);
    
    array_foreach_ref(HashmapItem, idx, item, this->buckets[bucket]) {
        StringView this_key = str_as_view(&item->key);
        if (sv_cmp(&this_key, &key) == 0) {
            if (this->object_freer) {
                this->object_freer(item->value);
            }

            str_free(&item->key);

            array_remove(HashmapItem, &this->buckets[bucket], idx);
            break;
        }
    }
}

void gramina_hashmap_free(Hashmap *this) {
    for (size_t i = 0; i < this->n_buckets; ++i) {
        array_foreach(HashmapItem, _, el, this->buckets[i]) {
            if (this->object_freer) {
                this->object_freer(el.value);
            }

            str_free(&el.key);
        }

        array_free(HashmapItem, &this->buckets[i]);
    }

    gramina_free(this->buckets);

    this->buckets = NULL;
    this->object_freer = NULL;
}

void gramina_hashmap_set_c(Hashmap *this, const char *key, void *value) {
    gramina_hashmap_set(this, mk_sv_c(key), value);
}

void *gramina_hashmap_get_c(const Hashmap *this, const char *key) {
    return gramina_hashmap_get(this, mk_sv_c(key));
}

void gramina_hashmap_remove_c(Hashmap *this, const char *key) {
    gramina_hashmap_remove(this, mk_sv_c(key));
}
