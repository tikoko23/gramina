#ifndef __GRAMINA_COMPILER_CSTATE_H
#define __GRAMINA_COMPILER_CSTATE_H

#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include "compiler.h"
#include "scope.h"

#include "common/stream.h"

#include "compiler/typedecl.h"

struct gramina_reflection {
    size_t depth;
    _GraminaType type;
};

typedef struct gramina_reflection _GraminaReflection;
GRAMINA_DECLARE_ARRAY(_GraminaReflection);

struct gramina_compiler_state {
    LLVMModuleRef llvm_module;
    LLVMBuilderRef llvm_builder;

    size_t reflection_depth;
    struct gramina_array(_GraminaReflection) reflection;
    struct gramina_array(GraminaScope) scopes;
    struct gramina_compile_error error;
    int status;
    bool has_error;
};

#define GRAMINA_CURRENT_SCOPE(S) (array_last(GraminaScope, &S->scopes))
#define GRAMINA_REFLECT(index) (S->reflection.items + (index))

#endif
#include "gen/compiler/cstate.h"

#if !defined(CURRENT_SCOPE) && defined(GRAMINA_NO_NAMESPACE)
#  define CURRENT_SCOPE(S) GRAMINA_CURRENT_SCOPE(S)
#endif

#if !defined(REFLECT) && defined(GRAMINA_NO_NAMESPACE)
#  define REFLECT(S) GRAMINA_REFLECT(S)
#endif
