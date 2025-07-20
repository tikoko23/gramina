#ifndef __GRAMINA_COMPILER_MEM_H
#define __GRAMINA_COMPILER_MEM_H

#include <llvm-c/Types.h>

#include "compiler/cstate.h"
#include "compiler/op.h"
#include "compiler/value.h"

void gramina_store(struct gramina_compiler_state *S, const struct gramina_value *value, LLVMValueRef into);

void gramina_try_load_inplace(struct gramina_compiler_state *S, struct gramina_value *val);
struct gramina_value gramina_try_load(struct gramina_compiler_state *S, const struct gramina_value *value);

struct gramina_value gramina_assign(struct gramina_compiler_state *S, struct gramina_value *target, const struct gramina_value *from, enum gramina_assign_op op);

struct gramina_value gramina_address_of(struct gramina_compiler_state *S, const struct gramina_value *operand);
struct gramina_value gramina_deref(struct gramina_compiler_state *S, const struct gramina_value *operand);

LLVMValueRef gramina_build_alloca(struct gramina_compiler_state *S, const struct gramina_type *type, const char *name);

size_t gramina_size_of(struct gramina_compiler_state *S, const struct gramina_type *type);
size_t gramina_align_of(struct gramina_compiler_state *S, const struct gramina_type *type);

#endif
#include "gen/compiler/mem.h"
