#define GRAMINA_NO_NAMESPACE

#include "compiler/arithmetic.h"
#include "compiler/conversions.h"
#include "compiler/errors.h"
#include "compiler/mem.h"

Value primitive_arithmetic(CompilerState *S, const Value *left, const Value *right, ArithmeticBinOp operation) {
    Value lhs;
    Value rhs;

    CoercionResult coerced = coerce_primitives(S, left, right, &lhs, &rhs);
    if (S->has_error) {
        return invalid_value();
    }

    LLVMOpcode op;

    bool is_integer = primitive_is_integral(coerced.greater_type.primitive);
    bool signedness = !primitive_is_unsigned(coerced.greater_type.primitive);

    switch (operation) {
    case GRAMINA_OP_ADD:
        op = is_integer
           ? LLVMAdd
           : LLVMFAdd;
        break;
    case GRAMINA_OP_SUB:
        op = is_integer
           ? LLVMSub
           : LLVMFSub;
        break;
    case GRAMINA_OP_MUL:
        op = is_integer
           ? LLVMMul
           : LLVMFMul;
        break;
    case GRAMINA_OP_DIV:
        if (is_integer) {
            op = signedness
               ? LLVMSDiv
               : LLVMUDiv;
        } else {
            op = LLVMFDiv;
        }
        break;
    case GRAMINA_OP_REM:
        if (is_integer) {
            op = signedness
               ? LLVMSRem
               : LLVMURem;
        } else {
            op = LLVMFRem;
        }
        break;
    default:
        return invalid_value();
    }

    String tstr = type_to_str(&coerced.greater_type);
    str_free(&tstr);

    Value ret = {
        .type = coerced.greater_type,
        .class = left->class == GRAMINA_CLASS_CONSTEXPR && right->class == GRAMINA_CLASS_CONSTEXPR
               ? GRAMINA_CLASS_CONSTEXPR
               : GRAMINA_CLASS_RVALUE,
    };

    ret.llvm = LLVMBuildBinOp(S->llvm_builder, op, lhs.llvm, rhs.llvm, "");

    return ret;
}

Value pointer_arithmetic(CompilerState *S, const Value *left, const Value *right, ArithmeticBinOp op) {
    if (op != GRAMINA_OP_ADD
     && op != GRAMINA_OP_SUB) {
        StringView op_str = get_arithmetic_bin_op(op);
        err_illegal_op(S, &left->type, &right->type, &op_str);

        return invalid_value();
    }

    // Both are pointers
    if (left->type.kind == right->type.kind) {
        if (op != GRAMINA_OP_SUB || !type_is_same(&left->type, &right->type)) {
            StringView op_str = get_arithmetic_bin_op(op);
            err_illegal_op(S, &left->type, &right->type, &op_str);

            return invalid_value();
        }

        Value lhs = pointer_to_int(S, left);
        Value rhs = pointer_to_int(S, right);

        LLVMValueRef diff_bytes = LLVMBuildSub(S->llvm_builder, lhs.llvm, rhs.llvm, "");
        LLVMValueRef elem_size = LLVMSizeOf(left->type.pointer_type->llvm);

        LLVMValueRef n_elems = LLVMBuildSDiv(S->llvm_builder, diff_bytes, elem_size, "");

        Value ret = {
            .llvm = n_elems,
            .type = type_from_primitive(GRAMINA_PRIMITIVE_LONG),
            .class = GRAMINA_CLASS_RVALUE,
        };

        value_free(&lhs);
        value_free(&rhs);

        return ret;
    }

    const Type *ptr_type = left->type.kind == GRAMINA_TYPE_POINTER
                         ? &left->type
                         : &right->type;

    const Value *ptr_val = left->type.kind == GRAMINA_TYPE_POINTER
                         ? left
                         : right;

    const Value *offset_val = left->type.kind == GRAMINA_TYPE_POINTER
                            ? right
                            : left;

    LLVMValueRef offset = offset_val->llvm;
    if (op == GRAMINA_OP_SUB) {
        offset = LLVMBuildNeg(S->llvm_builder, offset, "");
    }

    LLVMValueRef result = LLVMBuildGEP2(
        S->llvm_builder,
        ptr_type->pointer_type->llvm,
        ptr_val->llvm,
        &offset,
        1, ""
    );

    Value ret = {
        .llvm = result,
        .type = type_dup(ptr_type),
        .class = GRAMINA_CLASS_RVALUE,
    };

    return ret;
}

Value arithmetic(CompilerState *S, const Value *lhs, const Value *rhs, ArithmeticBinOp op) {
    if (lhs->type.kind == GRAMINA_TYPE_PRIMITIVE && rhs->type.kind == GRAMINA_TYPE_PRIMITIVE) {
        return primitive_arithmetic(S, lhs, rhs, op);
    }

    bool lhs_is_pointer_compatible = lhs->type.kind == GRAMINA_TYPE_POINTER;
    lhs_is_pointer_compatible |= lhs->type.kind == GRAMINA_TYPE_PRIMITIVE && primitive_is_integral(lhs->type.primitive);

    bool rhs_is_pointer_compatible = rhs->type.kind == GRAMINA_TYPE_POINTER;
    rhs_is_pointer_compatible |= rhs->type.kind == GRAMINA_TYPE_PRIMITIVE && primitive_is_integral(rhs->type.primitive);

    if (lhs_is_pointer_compatible && rhs_is_pointer_compatible) {
        Value loaded_left = try_load(S, lhs);
        Value loaded_right = try_load(S, rhs);

        Value ret = pointer_arithmetic(S, lhs, rhs, op);

        value_free(&loaded_left);
        value_free(&loaded_right);

        return ret;
    }

    return invalid_value();
}

Value unary_arithmetic(CompilerState *S, const Value *_operand, ArithmeticUnOp op) {
    Value operand = try_load(S, _operand);

    if (operand.type.kind == GRAMINA_TYPE_PRIMITIVE) {
        switch (op) {
        case GRAMINA_OP_IDENTITY: {
            Value ret = {
                .type = type_dup(&operand.type),
                .llvm = operand.llvm,
                .class = GRAMINA_CLASS_RVALUE,
            };

            value_free(&operand);

            return ret;
        }
        case GRAMINA_OP_NEGATION: {
            if (primitive_is_unsigned(operand.type.primitive)) {
                puts_err(S, str_cfmt("use of unary minus '-' on unsigned type"));
                S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
                return invalid_value();
            }

            LLVMValueRef result;
            if (primitive_is_integral(operand.type.primitive)) {
                result = LLVMBuildNeg(S->llvm_builder, operand.llvm, "");
            } else {
                result = LLVMBuildFNeg(S->llvm_builder, operand.llvm, "");
            }

            Value ret = {
                .type = type_dup(&operand.type),
                .llvm = result,
                .class = GRAMINA_CLASS_RVALUE,
            };

            value_free(&operand);

            return ret;
        }
        default:
            break;
        }
    }

    return invalid_value();
}
