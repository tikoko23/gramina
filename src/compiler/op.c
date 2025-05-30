#define GRAMINA_NO_NAMESPACE

#include "compiler/op.h"

int get_op_from_ast_node(const AstNode *node) {
    switch (node->type) {
    case GRAMINA_AST_OP_ADD:
        return GRAMINA_OP_ADD;
    case GRAMINA_AST_OP_SUB:
        return GRAMINA_OP_SUB;
    case GRAMINA_AST_OP_MUL:
        return GRAMINA_OP_MUL;
    case GRAMINA_AST_OP_DIV:
        return GRAMINA_OP_DIV;
    case GRAMINA_AST_OP_REM:
        return GRAMINA_OP_REM;

    case GRAMINA_AST_OP_UNARY_PLUS:
        return GRAMINA_OP_IDENTITY;
    case GRAMINA_AST_OP_UNARY_MINUS:
        return GRAMINA_OP_NEGATION;

    case GRAMINA_AST_OP_EQUAL:
        return GRAMINA_OP_EQUAL;
    case GRAMINA_AST_OP_INEQUAL:
        return GRAMINA_OP_INEQUAL;
    case GRAMINA_AST_OP_LT:
        return GRAMINA_OP_LT;
    case GRAMINA_AST_OP_LTE:
        return GRAMINA_OP_LTE;
    case GRAMINA_AST_OP_GT:
        return GRAMINA_OP_GT;
    case GRAMINA_AST_OP_GTE:
        return GRAMINA_OP_GTE;

    case GRAMINA_AST_OP_LOGICAL_OR:
        return GRAMINA_OP_L_OR;
    case GRAMINA_AST_OP_LOGICAL_XOR:
        return GRAMINA_OP_L_XOR;
    case GRAMINA_AST_OP_LOGICAL_AND:
        return GRAMINA_OP_L_AND;

    default:
        return -1;
    }
}

StringView get_arithmetic_bin_op(ArithmeticBinOp op) {
    switch (op) {
    case GRAMINA_OP_ADD:
        return mk_sv_c("+");
    case GRAMINA_OP_SUB:
        return mk_sv_c("-");
    case GRAMINA_OP_MUL:
        return mk_sv_c("*");
    case GRAMINA_OP_DIV:
        return mk_sv_c("/");
    case GRAMINA_OP_REM:
        return mk_sv_c("%");
    }
}

StringView get_comparison_op(ComparisonOp op) {
    switch (op) {
    case GRAMINA_OP_EQUAL:
        return mk_sv_c("==");
    case GRAMINA_OP_INEQUAL:
        return mk_sv_c("!=");
    case GRAMINA_OP_LT:
        return mk_sv_c("<");
    case GRAMINA_OP_LTE:
        return mk_sv_c("<=");
    case GRAMINA_OP_GT:
        return mk_sv_c(">");
    case GRAMINA_OP_GTE:
        return mk_sv_c(">=");
    }
}
