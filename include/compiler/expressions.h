#ifndef __GRAMINA_COMPILER_EXPRESSIONS_H
#define __GRAMINA_COMPILER_EXPRESSIONS_H

#include <llvm-c/Types.h>

#include "compiler/value.h"

struct gramina_value gramina_expression(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

struct gramina_value gramina_arithmetic_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);
struct gramina_value gramina_unary_arithmetic_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

struct gramina_value gramina_cast_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

struct gramina_value gramina_address_of_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);
struct gramina_value gramina_deref_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

struct gramina_value gramina_comparison_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

struct gramina_value gramina_binary_logic_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

struct gramina_value gramina_fn_call_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

struct gramina_value gramina_member_expr(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

#endif
#include "gen/compiler/expressions.h"