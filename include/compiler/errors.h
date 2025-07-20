#ifndef __GRAMINA_COMPILER_ERRORS_H
#define __GRAMINA_COMPILER_ERRORS_H

#include "compiler/cstate.h"

#include "parser/ast.h"

void gramina_putcs_err(struct gramina_compiler_state *S, const char *err);
void gramina_puts_err(struct gramina_compiler_state *S, struct gramina_string err);

void gramina_err_implicit_conv(struct gramina_compiler_state *S, const struct gramina_type *from, const struct gramina_type *to);
void gramina_err_explicit_conv(struct gramina_compiler_state *S, const struct gramina_type *from, const struct gramina_type *to);
void gramina_err_illegal_node(struct gramina_compiler_state *S, enum gramina_ast_node_type type);
void gramina_err_undeclared_ident(struct gramina_compiler_state *S, const struct gramina_string_view *name);
void gramina_err_redeclaration(struct gramina_compiler_state *S, const struct gramina_string_view *name);
void gramina_err_rvalue_assign(struct gramina_compiler_state *S, const struct gramina_type *type);
void gramina_err_deref(struct gramina_compiler_state *S, const struct gramina_type *type);
void gramina_err_illegal_op(struct gramina_compiler_state *S, const struct gramina_type *l, const struct gramina_type *r, const struct gramina_string_view *op);
void gramina_err_missing_ret(struct gramina_compiler_state *S, const struct gramina_type *ret);
void gramina_err_cannot_call(struct gramina_compiler_state *S, const struct gramina_type *type);
void gramina_err_cannot_have_member(struct gramina_compiler_state *S, const struct gramina_type *type);
void gramina_err_cannot_have_prop(struct gramina_compiler_state *S, const struct gramina_type *type);
void gramina_err_no_field(struct gramina_compiler_state *S, const struct gramina_type *type, const struct gramina_string_view *field);
void gramina_err_no_getprop(struct gramina_compiler_state *S, const struct gramina_type *type, const struct gramina_string_view *prop);
void gramina_err_insufficient_args(struct gramina_compiler_state *S, size_t wants, size_t got);
void gramina_err_excess_args(struct gramina_compiler_state *S, size_t wants);
void gramina_err_no_attrib_arg(struct gramina_compiler_state *S, const struct gramina_string_view *attrib_name);
void gramina_err_const_assign(struct gramina_compiler_state *S, const struct gramina_type *type);
void gramina_err_discard_const(struct gramina_compiler_state *S, const struct gramina_type *from, const struct gramina_type *to);
void gramina_err_bad_type(struct gramina_compiler_state *S, const struct gramina_type *type);

#endif
#include "gen/compiler/errors.h"
