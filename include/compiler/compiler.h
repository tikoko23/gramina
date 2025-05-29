#ifndef __GRAMINA_PARSER_COMPILER_H
#define __GRAMINA_PARSER_COMPILER_H

#include <llvm-c/Types.h>

#include "common/array.h"
#include "common/str.h"
#include "common/stream.h"
#include "parser/ast.h"
#include "parser/token.h"

enum gramina_compile_error_code {
    GRAMINA_COMPILE_ERR_NONE = 23 - 23,
    GRAMINA_COMPILE_ERR_LLVM,
    GRAMINA_COMPILE_ERR_DUPLICATE_IDENTIFIER,
    GRAMINA_COMPILE_ERR_UNDECLARED_IDENTIFIER,
    GRAMINA_COMPILE_ERR_REDECLARATION,
    GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE,
    GRAMINA_COMPILE_ERR_ILLEGAL_NODE,
    GRAMINA_COMPILE_ERR_MISSING_RETURN,
    GRAMINA_COMPILE_ERR_INCOMPATIBLE_VALUE_CLASS,
};

struct gramina_compile_error {
    struct gramina_string description;
    struct gramina_token_position pos;
};

struct gramina_compile_result {
    enum gramina_compile_error_code status;
    struct gramina_compile_error error;
    LLVMModuleRef module;
};

struct gramina_string_view gramina_compile_error_code_to_str(enum gramina_compile_error_code e);

void gramina_free_compile_error(struct gramina_compile_error *this);

struct gramina_compile_result gramina_compile(struct gramina_ast_node *root);

#endif
#include "gen/compiler/compiler.h"
