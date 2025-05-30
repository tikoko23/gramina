#ifndef __GRAMINA_COMPILER_ARITHMETIC_H
#define __GRAMINA_COMPILER_ARITHMETIC_H

#include "compiler/cstate.h"
#include "compiler/op.h"
#include "compiler/value.h"

struct gramina_value gramina_pointer_arithmetic(struct gramina_compiler_state *S, const struct gramina_value *left, const struct gramina_value *right, enum gramina_arithmetic_bin_op op);

struct gramina_value gramina_primitive_arithmetic(struct gramina_compiler_state *S, const struct gramina_value *left, const struct gramina_value *right, enum gramina_arithmetic_bin_op op);

struct gramina_value gramina_arithmetic(struct gramina_compiler_state *S, const struct gramina_value *left, const struct gramina_value *right, enum gramina_arithmetic_bin_op op);

struct gramina_value gramina_unary_arithmetic(struct gramina_compiler_state *S, const struct gramina_value *operand, enum gramina_arithmetic_un_op op);

#endif
#include "gen/compiler/arithmetic.h"