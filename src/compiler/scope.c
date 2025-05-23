#define GRAMINA_NO_NAMESPACE

#include <llvm-c/Types.h>

#include "compiler/identifier.h"
#include "compiler/scope.h"
#include "compiler/type.h"

#define GRAMINA_SCOPE_BUCKETS 128

GRAMINA_IMPLEMENT_ARRAY(GraminaScope)

static void free_identifier(void *ident_ptr) {
    Identifier *ident = ident_ptr;

    identifier_free(ident);
    gramina_free(ident);
}

Scope gramina_mk_scope() {
    Scope this = {};

    this.identifiers = mk_hashmap(128);
    this.identifiers.object_freer = free_identifier;

    return this;
}

#define DECLARE_PRIMITIVE(name) \
do { \
    builtins[__i]->kind = GRAMINA_IDENT_KIND_TYPE; \
    builtins[__i]->type = (Type) { \
        .kind = GRAMINA_TYPE_PRIMITIVE, \
        .primitive = GRAMINA_PRIMITIVE_ ## name, \
    }; \
    ++__i; \
} while (GRAMINA_ZERO) \

void gramina_scope_free(Scope *this) {
    hashmap_free(&this->identifiers);
}

