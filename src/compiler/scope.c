#define GRAMINA_NO_NAMESPACE

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

void gramina_scope_free(Scope *this) {
    hashmap_free(&this->identifiers);
}

