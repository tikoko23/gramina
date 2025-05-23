#ifndef __GRAMINA_COMPILER_SCOPE_H

#include "common/hashmap.h"
#include "common/array.h"

struct gramina_scope {
    struct gramina_hashmap identifiers;
};

#define GRAMINA_WANT_TAGLESS
#endif
#include "gen/compiler/scope.h"
#ifndef __GRAMINA_COMPILER_SCOPE_H
#define __GRAMINA_COMPILER_SCOPE_H

#undef GRAMINA_WANT_TAGLESS

GRAMINA_DECLARE_ARRAY(GraminaScope)

struct gramina_scope gramina_mk_scope();

void gramina_scope_free(struct gramina_scope *this);

#endif
