#ifndef __GRAMINA_COMPILER_CONVERSIONS_H
#define __GRAMINA_COMPILER_CONVERSIONS_H

#include "compiler/cstate.h"
#include "compiler/value.h"

struct gramina_value gramina_primitive_convert(struct gramina_compiler_state *S, const struct gramina_value *from, const struct gramina_type *to);
bool gramina_convert_inplace(struct gramina_compiler_state *S, struct gramina_value *value, const struct gramina_type *to);
struct gramina_value gramina_convert(struct gramina_compiler_state *S, const struct gramina_value *from, const struct gramina_type *to);

struct gramina_coercion_result gramina_coerce_primitives(struct gramina_compiler_state *S, const struct gramina_value *left, const struct gramina_value *right, struct gramina_value *result_lhs, struct gramina_value *result_rhs);

struct gramina_value gramina_pointer_to_int(struct gramina_compiler_state *S, const struct gramina_value *ptr);

struct gramina_value gramina_cast(struct gramina_compiler_state *S, const struct gramina_value *from, const struct gramina_type *to);

#endif
#include "gen/compiler/conversions.h"
