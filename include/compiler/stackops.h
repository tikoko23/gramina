#ifndef __GRAMINA_COMPILER_STACKOPS_H
#define __GRAMINA_COMPILER_STACKOPS_H

#include "compiler/cstate.h"

struct gramina_scope *gramina_push_scope(struct gramina_compiler_state *S);
void gramina_pop_scope(struct gramina_compiler_state *S);

void gramina_push_reflection(struct gramina_compiler_state *S, const struct gramina_type *type);
void gramina_pop_reflection(struct gramina_compiler_state *S);

#endif
#include "gen/compiler/stackops.h"