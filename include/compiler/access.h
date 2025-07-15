#ifndef __GRAMINA_COMPILER_ACCESS_H
#define __GRAMINA_COMPILER_ACCESS_H

#include "compiler/cstate.h"
#include "compiler/value.h"

struct gramina_value gramina_subscript(struct gramina_compiler_state *S, const struct gramina_value *scriptee, const struct gramina_value *scripter);

struct gramina_value gramina_member(struct gramina_compiler_state *S, const struct gramina_value *operand, const struct gramina_string_view *field_name);

#endif
#include "gen/compiler/access.h"
