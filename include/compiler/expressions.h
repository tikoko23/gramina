#ifndef __GRAMINA_COMPILER_EXPRESSIONS_H
#define __GRAMINA_COMPILER_EXPRESSIONS_H

#include <llvm-c/Types.h>

#include "compiler/value.h"

struct gramina_value gramina_expression(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

struct gramina_value gramina_arithmetic_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);
struct gramina_value gramina_unary_arithmetic_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

struct gramina_value gramina_cast_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

struct gramina_value gramina_assign_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

struct gramina_value gramina_address_of_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);
struct gramina_value gramina_deref_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

struct gramina_value gramina_comparison_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

struct gramina_value gramina_binary_skipped_logic_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

struct gramina_value gramina_fn_call_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

struct gramina_value gramina_member_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);
struct gramina_value gramina_subscript_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);
struct gramina_value get_property_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *this);

struct gramina_value gramina_size_of_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *this);
struct gramina_value gramina_align_of_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *this);

#endif
#include "gen/compiler/expressions.h"
