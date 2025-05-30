#ifndef __GRAMINA_COMPILER_STRUCT_H
#define __GRAMINA_COMPILER_STRUCT_H

#include "compiler/cstate.h"
#include "compiler/value.h"

void gramina_struct_def(struct gramina_compiler_state *S, struct gramina_ast_node *node);

struct gramina_value gramina_member(struct gramina_compiler_state *S, const struct gramina_value *operand, const struct gramina_string_view *field_name);

#endif
