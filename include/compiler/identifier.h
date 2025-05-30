#ifndef __GRAMINA_COMPILER_IDENTIFIER_H

#include <llvm-c/Types.h>

#include "common/array.h"
#include "common/str.h"

#include "compiler/cstate.h"
#include "compiler/type.h"

#include "parser/attributes.h"

enum gramina_identifier_kind {
    GRAMINA_IDENT_KIND_VAR,
    GRAMINA_IDENT_KIND_FUNC,
    GRAMINA_IDENT_KIND_TYPE,
};

struct gramina_identifier {
    enum gramina_identifier_kind kind;
    struct gramina_type type;
    struct gramina_array(_GraminaSymAttr) attributes;
    LLVMValueRef llvm;
};

#define GRAMINA_WANT_TAGLESS
#endif
#include "gen/compiler/identifier.h"
#ifndef __GRAMINA_COMPILER_IDENTIFIER_H
#define __GRAMINA_COMPILER_IDENTIFIER_H

#undef GRAMINA_WANT_TAGLESS

GRAMINA_DECLARE_ARRAY(GraminaIdentifier);

struct gramina_identifier *gramina_scope_resolve(const struct gramina_scope *this, const struct gramina_string_view *ident_name);
struct gramina_identifier *gramina_resolve(const struct gramina_compiler_state *S, const struct gramina_string_view *ident_name);

void gramina_identifier_free(struct gramina_identifier *this);

#endif
