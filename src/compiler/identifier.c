#define GRAMINA_NO_NAMESPACE

#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include "compiler/identifier.h"
#include "compiler/type.h"

#include "parser/attributes.h"

GRAMINA_IMPLEMENT_ARRAY(GraminaIdentifier)

Identifier *gramina_scope_resolve(const Scope *this, const StringView *ident_name) {
    return hashmap_get(&this->identifiers, *ident_name);
}

Identifier *gramina_resolve(const CompilerState *S, const StringView *ident_name) {
    if (S->scopes.length == 0) {
        return NULL;
    }

    for (int32_t i = S->scopes.length - 1; i >= 0; --i) {
        Identifier *candidate = scope_resolve(&S->scopes.items[i], ident_name);

        if (candidate) {
            return candidate;
        }
    }

    return NULL;
}


void gramina_identifier_free(Identifier *this) {
    type_free(&this->type);
    this->type = (Type) {
        .kind = GRAMINA_TYPE_INVALID,
    };

    array_foreach_ref(_GraminaSymAttr, _, attrib, this->attributes) {
        symattr_free(attrib);
    }

    array_free(_GraminaSymAttr, &this->attributes);
}

