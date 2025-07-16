#ifndef __GRAMINA_COMPILER_TYPE_H
#define __GRAMINA_COMPILER_TYPE_H

#include "compiler/cstate.h"

bool gramina_kind_is_aggregate(enum gramina_type_kind k);

struct gramina_type gramina_mk_pointer_type(const struct gramina_type *pointed);
struct gramina_type gramina_mk_array_type(const struct gramina_type *element, size_t length);
struct gramina_type gramina_mk_slice_type(const struct gramina_type *element);

struct gramina_type gramina_type_from_primitive(enum gramina_primitive this);
struct gramina_type gramina_type_from_ast_node(struct gramina_compiler_state *S, const struct gramina_ast_node *node);
struct gramina_string gramina_type_to_str(const struct gramina_type *this);

struct gramina_type gramina_type_dup(const struct gramina_type *this);

bool gramina_type_is_same(const struct gramina_type *a, const struct gramina_type *b);

bool gramina_type_can_convert(const struct gramina_compiler_state *S, const struct gramina_type *from, const struct gramina_type *to);
bool gramina_init_respects_constness(const struct gramina_compiler_state *S, const struct gramina_type *from, const struct gramina_type *to);

struct gramina_type gramina_decltype(const struct gramina_compiler_state *S, const struct gramina_ast_node *exp);

bool gramina_primitive_is_unsigned(enum gramina_primitive this);
bool gramina_primitive_is_integral(enum gramina_primitive this);
bool gramina_primitive_can_convert(enum gramina_primitive a, enum gramina_primitive to);
struct gramina_coercion_result gramina_primitive_coercion(const struct gramina_type *a, const struct gramina_type *b);

void gramina_type_free(struct gramina_type *this);

#endif
#include "gen/compiler/type.h"
