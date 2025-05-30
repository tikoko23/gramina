#ifndef __GRAMINA_COMPILER_DECL_H
#define __GRAMINA_COMPILER_DECL_H

#include <llvm-c/Types.h>

#include "compiler/type.h"

enum gramina_value_class {
    GRAMINA_CLASS_INVALID,
    GRAMINA_CLASS_ALLOCA,
    GRAMINA_CLASS_LVALUE,
    GRAMINA_CLASS_RVALUE,
    GRAMINA_CLASS_CONSTEXPR,
};

struct gramina_value {
    union {
        LLVMValueRef lvalue_ptr;
    };

    enum gramina_value_class class;
    LLVMValueRef llvm;
    struct gramina_type type;
};

bool gramina_value_is_valid(const struct gramina_value *this);
void gramina_value_free(struct gramina_value *this);
struct gramina_value gramina_value_dup(const struct gramina_value *this);
struct gramina_value gramina_invalid_value();

#endif
#include "gen/compiler/value.h"