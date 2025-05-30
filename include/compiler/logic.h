#ifndef __GRAMINA_COMPILER_COMPARISON_H
#define __GRAMINA_COMPILER_COMPARISON_H

#include "compiler/op.h"
#include "compiler/value.h"

struct gramina_value gramina_primitive_comparison(struct gramina_compiler_state *S, const struct gramina_value *lhs, const struct gramina_value *rhs, enum gramina_comparison_op op);

struct gramina_value gramina_comparison(struct gramina_compiler_state *S, const struct gramina_value *lhs, const struct gramina_value *rhs, enum gramina_comparison_op op);

struct gramina_value gramina_binary_logic(struct gramina_compiler_state *S, const struct gramina_value *lhs, const struct gramina_value *rhs, enum gramina_logical_bin_op op);

#endif
#include "gen/compiler/logic.h"