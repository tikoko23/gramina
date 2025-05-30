#ifndef __GRAMINA_COMPILER_MEM_H
#define __GRAMINA_COMPILER_MEM_H

#include "compiler/cstate.h"
#include "compiler/value.h"
#include <llvm-c/Types.h>

void gramina_store(struct gramina_compiler_state *S, const struct gramina_value *value, LLVMValueRef into);

void gramina_try_load_inplace(struct gramina_compiler_state *S, struct gramina_value *val);
struct gramina_value gramina_try_load(struct gramina_compiler_state *S, const struct gramina_value *value);

struct gramina_value gramina_assign(struct gramina_compiler_state *S, struct gramina_value *target, const struct gramina_value *from);

struct gramina_value gramina_address_of(struct gramina_compiler_state *S, const struct gramina_value *operand);
struct gramina_value gramina_deref(struct gramina_compiler_state *S, const struct gramina_value *operand);

#endif
#include "gen/compiler/mem.h"
