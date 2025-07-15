#define GRAMINA_NO_NAMESPACE

#include "common/log.h"

#include "compiler/errors.h"
#include "compiler/identifier.h"
#include "compiler/mem.h"
#include "compiler/scope.h"
#include "compiler/struct.h"

void struct_def(CompilerState *S, AstNode *this) {
    Type struct_type = type_from_ast_node(S, this);

    Identifier *ident = gramina_malloc(sizeof *ident);
    *ident = (Identifier) {
        .llvm = NULL,
        .type = struct_type,
        .kind = GRAMINA_IDENT_KIND_TYPE,
    };

    StringView name = str_as_view(&ident->type.struct_name);

    Scope *scope = CURRENT_SCOPE(S);
    hashmap_set(&scope->identifiers, name, ident);

    vlog_fmt("Registering struct '{sv}'\n", &name);
}
