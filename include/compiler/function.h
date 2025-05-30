#ifndef __GRAMINA_COMPILER_FNUCTION_H
#define __GRAMINA_COMPILER_FNUCTION_H

#include "compiler/cstate.h"
#include "compiler/identifier.h"
#include "compiler/value.h"

void gramina_function_def(struct gramina_compiler_state *S, struct gramina_ast_node *node);

struct gramina_value gramina_call(struct gramina_compiler_state *S, const struct gramina_identifier *func, const struct gramina_value *args, size_t n_args);

#endif
