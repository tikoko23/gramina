#define GRAMINA_NO_NAMESPACE

#include "compiler/stackops.h"
#include "compiler/type.h"

Scope *push_scope(CompilerState *S) {
    array_append(GraminaScope, &S->scopes, mk_scope());
    return array_last(GraminaScope, &S->scopes);
}

void pop_scope(CompilerState *S) {
    scope_free(array_last(GraminaScope, &S->scopes));
    array_pop(GraminaScope, &S->scopes);
}

void push_reflection(CompilerState *S, const Type *type) {
    array_append(_GraminaReflection, &S->reflection, ((Reflection) {
        .type = type_dup(type),
        .depth = S->reflection_depth
    }));
}

void pop_reflection(CompilerState *S) {
    type_free(&array_last(_GraminaReflection, &S->reflection)->type);
    array_pop(_GraminaReflection, &S->reflection);
}
