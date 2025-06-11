#ifndef __GRAMINA_COMPILER_ACCESS_H
#define __GRAMINA_COMPILER_ACCESS_H

#include "compiler/cstate.h"
#include "compiler/value.h"

struct gramina_value gramina_subscript(struct gramina_compiler_state *S, const struct gramina_value *scriptee, const struct gramina_value *scripter);

#endif
#include "gen/compiler/access.h"