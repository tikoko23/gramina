#define GRAMINA_NO_NAMESPACE

#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/Types.h>

#include "compiler/arithmetic.h"
#include "compiler/conversions.h"
#include "compiler/errors.h"
#include "compiler/mem.h"
#include "compiler/op.h"

void store(CompilerState *S, const Value *value, LLVMValueRef into) {
    size_t align = LLVMABIAlignmentOfType(S->llvm_target_data, value->type.llvm);
    if (kind_is_aggregate(value->type.kind)) {
        LLVMBuildMemCpy(S->llvm_builder, into, align, value->llvm, align, LLVMSizeOf(value->type.llvm));
    } else {
        LLVMBuildStore(S->llvm_builder, value->llvm, into);
    }
}

void try_load_inplace(CompilerState *S, Value *val) {
    if (val->class != GRAMINA_CLASS_ALLOCA) {
        return;
    }

    if (kind_is_aggregate(val->type.kind)) {
        Value new_val = {
            .llvm = val->llvm,
            .class = GRAMINA_CLASS_LVALUE,
            .type = type_dup(&val->type),
            .lvalue_ptr = val->llvm,
        };

        value_free(val);

        *val = new_val;

        return;
    }

    LLVMValueRef loaded = LLVMBuildLoad2(S->llvm_builder, val->type.llvm, val->llvm, "");

    Value new_val = {
        .llvm = loaded,
        .lvalue_ptr = val->llvm,
        .class = GRAMINA_CLASS_LVALUE,
        .type = type_dup(&val->type),
    };

    value_free(val);

    *val = new_val;
}

Value try_load(CompilerState *S, const Value *val) {
    if (val->class != GRAMINA_CLASS_ALLOCA) {
        return value_dup(val);
    }

    if (kind_is_aggregate(val->type.kind)) {
        return (Value) {
            .llvm = val->llvm,
            .class = GRAMINA_CLASS_LVALUE,
            .type = type_dup(&val->type),
            .lvalue_ptr = val->llvm,
        };
    }

    LLVMValueRef loaded = LLVMBuildLoad2(S->llvm_builder, val->type.llvm, val->llvm, "");

    return (Value) {
        .llvm = loaded,
        .lvalue_ptr = val->llvm,
        .class = GRAMINA_CLASS_LVALUE,
        .type = type_dup(&val->type),
    };
}


Value address_of(CompilerState *S, const Value *operand) {
    switch (operand->class) {
    case GRAMINA_CLASS_ALLOCA: {
        Value ret = {
            .class = GRAMINA_CLASS_RVALUE,
            .type = mk_pointer_type(&operand->type),
            .llvm = operand->llvm,
        };

        return ret;
    }
    case GRAMINA_CLASS_LVALUE: {
        Value ret = {
            .class = GRAMINA_CLASS_RVALUE,
            .type = mk_pointer_type(&operand->type),
            .llvm = operand->lvalue_ptr,
        };

        return ret;
    }
    default:
        puts_err(S, str_cfmt("taking address of rvalue of type '{so}'", type_to_str(&operand->type)));
        return invalid_value();
    }
}

Value deref(CompilerState *S, const Value *_operand) {
    Value operand = try_load(S, _operand);

    if (operand.type.kind != GRAMINA_TYPE_POINTER) {
        err_deref(S, &operand.type);
        value_free(&operand);
        return invalid_value();
    }

    bool ptr_to_aggregate = kind_is_aggregate(operand.type.pointer_type->kind);
    if (ptr_to_aggregate) {
        Value ret = {
            .type = type_dup(operand.type.pointer_type),
            .llvm = operand.llvm,
            .class = GRAMINA_CLASS_LVALUE,
            .lvalue_ptr = operand.llvm,
        };

        value_free(&operand);

        return ret;
    }

    Type *final_type = operand.type.pointer_type;
    LLVMValueRef read_val = LLVMBuildLoad2(S->llvm_builder, final_type->llvm, operand.llvm, "");

    Value ret = {
        .llvm = read_val,
        .lvalue_ptr = operand.llvm,
        .type = type_dup(final_type),
        .class = GRAMINA_CLASS_LVALUE,
    };

    value_free(&operand);
    return ret;
}

Value assign(CompilerState *S, Value *target, const Value *from, AssignOp op) {
    LLVMValueRef ptr;
    switch (target->class) {
    case GRAMINA_CLASS_ALLOCA:
        ptr = target->llvm;
        break;
    case GRAMINA_CLASS_LVALUE:
        ptr = target->lvalue_ptr;
        break;
    default:
        err_rvalue_assign(S, &target->type);
        return invalid_value();
    }

    if (target->type.is_const) {
        err_const_assign(S, &target->type);
        return invalid_value();
    }

    if (!init_respects_constness(S, &from->type, &target->type)) {
        err_discard_const(S, &from->type, &target->type);
        return invalid_value();
    }


    if (!type_can_convert(S, &from->type, &target->type)) {
        err_implicit_conv(S, &from->type, &target->type);
        return invalid_value();
    }

    ArithmeticBinOp aop;

    switch (op) {
    case GRAMINA_OP_ASSIGN: {
        Value loaded = try_load(S, from);
        convert_inplace(S, &loaded, &target->type);

        store(S, &loaded, ptr);
        // LLVMBuildStore(S->llvm_builder, value.llvm, ptr);

        loaded.class = GRAMINA_CLASS_RVALUE;

        return loaded;
    }
    case GRAMINA_OP_A_CAT: {
        StringView op = mk_sv_c("~=");
        err_illegal_op(S, &target->type, &from->type, &op);
        return invalid_value();
    }
    case GRAMINA_OP_A_ADD:
        aop = GRAMINA_OP_ADD;
        break;
    case GRAMINA_OP_A_SUB:
        aop = GRAMINA_OP_SUB;
        break;
    case GRAMINA_OP_A_MUL:
        aop = GRAMINA_OP_MUL;
        break;
    case GRAMINA_OP_A_DIV:
        aop = GRAMINA_OP_DIV;
        break;
    case GRAMINA_OP_A_REM:
        aop = GRAMINA_OP_REM;
        break;
    }

    Value result = arithmetic(S, target, from, aop);
    Value ret = assign(S, target, &result, GRAMINA_OP_ASSIGN);

    value_free(&result);
    return ret;
}

LLVMValueRef build_alloca(CompilerState *S, const Type *type, const char *name) {
    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(S->llvm_builder);
    LLVMBasicBlockRef first_block = current_block;
    while (true) {
        LLVMBasicBlockRef above = LLVMGetPreviousBasicBlock(first_block);
        if (!above) {
            break;
        }

        first_block = above;
    }

    LLVMPositionBuilderAtEnd(S->llvm_builder, first_block);

    LLVMValueRef ret = LLVMBuildAlloca(S->llvm_builder, type->llvm, name);

    if (type->kind == GRAMINA_TYPE_ARRAY) {
        LLVMSetAlignment(ret, 16);
    }

    LLVMPositionBuilderAtEnd(S->llvm_builder, current_block);

    return ret;
}

size_t size_of(struct gramina_compiler_state *S, const struct gramina_type *type) {
    return LLVMABISizeOfType(S->llvm_target_data, type->llvm);
}

size_t align_of(struct gramina_compiler_state *S, const struct gramina_type *type) {
    return LLVMABIAlignmentOfType(S->llvm_target_data, type->llvm);
}
