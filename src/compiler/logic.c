#define GRAMINA_NO_NAMESPACE

#include "compiler/conversions.h"
#include "compiler/errors.h"
#include "compiler/logic.h"
#include "compiler/mem.h"
#include "compiler/op.h"

Value primitive_comparison(CompilerState *S, const Value *left, const Value *right, ComparisonOp operation) {
    Value lhs;
    Value rhs;

    CoercionResult coerced = coerce_primitives(S, left, right, &lhs, &rhs);
    if (!value_is_valid(&lhs) || !value_is_valid(&rhs) || coerced.greater_type.kind == GRAMINA_TYPE_INVALID) {
        return invalid_value();
    }

    bool signedness = !primitive_is_unsigned(coerced.greater_type.primitive);
    bool integral = primitive_is_integral(coerced.greater_type.primitive);

    LLVMValueRef result;

    if (integral) {
        LLVMIntPredicate op;
        switch (operation) {
        case GRAMINA_OP_EQUAL:
            op = LLVMIntEQ;
            break;
        case GRAMINA_OP_INEQUAL:
            op = LLVMIntNE;
            break;
        case GRAMINA_OP_GT:
            op = signedness
               ? LLVMIntSGT
               : LLVMIntUGT;
            break;
        case GRAMINA_OP_GTE:
            op = signedness
               ? LLVMIntSGE
               : LLVMIntUGE;
            break;
        case GRAMINA_OP_LT:
            op = signedness
               ? LLVMIntSLT
               : LLVMIntULT;
            break;
        case GRAMINA_OP_LTE:
            op = signedness
               ? LLVMIntSLE
               : LLVMIntULE;
            break;
        default:
            break;
        }

        result = LLVMBuildICmp(S->llvm_builder, op, lhs.llvm, rhs.llvm, "");
    } else {
        LLVMRealPredicate op;
        switch (operation) {
        case GRAMINA_OP_EQUAL:
            op = LLVMRealOEQ;
            break;
        case GRAMINA_OP_INEQUAL:
            op = LLVMRealONE;
            break;
        case GRAMINA_OP_GT:
            op = LLVMRealOGT;
            break;
        case GRAMINA_OP_GTE:
            op = LLVMRealOGE;
            break;
        case GRAMINA_OP_LT:
            op = LLVMRealOLT;
            break;
        case GRAMINA_OP_LTE:
            op = LLVMRealOLE;
            break;
        default:
            break;
        }

        result = LLVMBuildFCmp(S->llvm_builder, op, lhs.llvm, rhs.llvm, "");
    }

    Value ret = {
        .type = type_from_primitive(GRAMINA_PRIMITIVE_BOOL),
        .llvm = result,
        .class = GRAMINA_CLASS_RVALUE,
    };

    value_free(&lhs);
    value_free(&rhs);

    type_free(&coerced.greater_type);

    return ret;
}

Value comparison(CompilerState *S, const Value *lhs, const Value *rhs, ComparisonOp operation) {
    if (lhs->type.kind == GRAMINA_TYPE_PRIMITIVE && rhs->type.kind == GRAMINA_TYPE_PRIMITIVE) {
        Value left = try_load(S, lhs);
        Value right = try_load(S, rhs);

        Value ret = primitive_comparison(S, &left, &right, operation);

        value_free(&left);
        value_free(&right);

        return ret;
    }

    if (lhs->type.kind == GRAMINA_TYPE_POINTER && rhs->type.kind == GRAMINA_TYPE_POINTER) {
        if (!type_is_same(lhs->type.pointer_type, rhs->type.pointer_type)) {
            StringView op_str = get_comparison_op(operation);
            err_illegal_op(S, &lhs->type, &rhs->type, &op_str);

            return invalid_value();
        }

        LLVMIntPredicate op;
        switch (operation) {
        case GRAMINA_OP_EQUAL:
            op = LLVMIntEQ;
            break;
        case GRAMINA_OP_INEQUAL:
            op = LLVMIntNE;
            break;
        case GRAMINA_OP_GT:
            op = LLVMIntUGT;
            break;
        case GRAMINA_OP_GTE:
            op = LLVMIntUGE;
            break;
        case GRAMINA_OP_LT:
            op = LLVMIntULT;
            break;
        case GRAMINA_OP_LTE:
            op = LLVMIntULE;
            break;
        default:
            break;
        }

        Value left = try_load(S, lhs);
        Value right = try_load(S, rhs);

        LLVMValueRef result = LLVMBuildICmp(S->llvm_builder, op, left.llvm, right.llvm, "");
        Value ret = {
            .llvm = result,
            .type = type_from_primitive(GRAMINA_PRIMITIVE_BOOL),
            .class = GRAMINA_CLASS_RVALUE,
        };

        value_free(&left);
        value_free(&right);

        return ret;
    }

    return invalid_value();
}

Value binary_logic(CompilerState *S, const Value *_lhs, const Value *_rhs, LogicalBinOp operation) {
    Value lhs = try_load(S, _lhs);
    Value rhs = try_load(S, _rhs);

    Type bool_type = type_from_primitive(GRAMINA_PRIMITIVE_BOOL);

    if (!type_can_convert(S, &lhs.type, &bool_type)) {
        err_implicit_conv(S, &lhs.type, &bool_type);

        type_free(&bool_type);
        value_free(&lhs);
        value_free(&rhs);

        return invalid_value();
    }

    if (!type_can_convert(S, &rhs.type, &bool_type)) {
        err_implicit_conv(S, &rhs.type, &bool_type);

        type_free(&bool_type);
        value_free(&lhs);
        value_free(&rhs);

        return invalid_value();
    }

    convert_inplace(S, &lhs, &bool_type);
    convert_inplace(S, &rhs, &bool_type);

    try_load_inplace(S, &lhs);
    try_load_inplace(S, &rhs);

    LLVMValueRef result;

    switch (operation) {
    case GRAMINA_OP_L_OR:
        result = LLVMBuildOr(S->llvm_builder, lhs.llvm, rhs.llvm, "");
        break;
    case GRAMINA_OP_L_XOR:
        result = LLVMBuildXor(S->llvm_builder, lhs.llvm, rhs.llvm, "");
        break;
    case GRAMINA_OP_L_AND:
        result = LLVMBuildAnd(S->llvm_builder, lhs.llvm, rhs.llvm, "");
        break;
    default:
        type_free(&bool_type);

        value_free(&lhs);
        value_free(&rhs);

        return invalid_value();
    }

    Value ret = {
        .llvm = result,
        .type = bool_type,
        .class = GRAMINA_CLASS_RVALUE,
    };

    value_free(&lhs);
    value_free(&rhs);

    return ret;
}
