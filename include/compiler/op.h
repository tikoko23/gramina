#ifndef __GRAMINA_COMPILER_OP_H
#define __GRAMINA_COMPILER_OP_H

#include "common/str.h"

#include "parser/ast.h"

enum gramina_arithmetic_bin_op {
    GRAMINA_OP_ADD,
    GRAMINA_OP_SUB,
    GRAMINA_OP_MUL,
    GRAMINA_OP_DIV,
    GRAMINA_OP_REM,
};

enum gramina_arithmetic_un_op {
    GRAMINA_OP_IDENTITY = GRAMINA_OP_REM + 1,
    GRAMINA_OP_NEGATION,
};

enum gramina_assign_op {
    GRAMINA_OP_ASSIGN = GRAMINA_OP_NEGATION + 1,
    GRAMINA_OP_A_ADD,
    GRAMINA_OP_A_SUB,
    GRAMINA_OP_A_MUL,
    GRAMINA_OP_A_DIV,
    GRAMINA_OP_A_REM,
    GRAMINA_OP_A_CAT,
};

enum gramina_comparison_op {
    GRAMINA_OP_EQUAL = GRAMINA_OP_A_CAT + 1,
    GRAMINA_OP_INEQUAL,
    GRAMINA_OP_LT,
    GRAMINA_OP_LTE,
    GRAMINA_OP_GT,
    GRAMINA_OP_GTE,
};

enum gramina_logical_bin_op {
    GRAMINA_OP_L_OR = GRAMINA_OP_GTE + 1,
    GRAMINA_OP_L_XOR,
    GRAMINA_OP_L_AND,
};

int gramina_get_op_from_ast_node(const struct gramina_ast_node *node);
struct gramina_string_view gramina_get_arithmetic_bin_op(enum gramina_arithmetic_bin_op op);
struct gramina_string_view gramina_get_comparison_op(enum gramina_comparison_op op);

#endif
#include "gen/compiler/op.h"
