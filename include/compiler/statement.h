#ifndef __GRAMINA_COMPILER_STATEMENT_H
#define __GRAMINA_COMPILER_STATEMENT_H

#include "compiler/cstate.h"
#include "compiler/value.h"
#include <llvm-c/Types.h>

struct gramina_identifier *gramina_declaration(struct gramina_compiler_state *S, const struct gramina_string_view *name, const struct gramina_type *type, const struct gramina_value *init);

void gramina_declaration_statement(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

void gramina_return_statement(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

void gramina_if_statement(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

void gramina_while_statement(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

void gramina_for_statement(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

bool gramina_block(struct gramina_compiler_state *S, LLVMValueRef function, struct gramina_ast_node *node);

#endif
#include "gen/compiler/statement.h"
