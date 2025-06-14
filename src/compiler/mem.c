#define GRAMINA_NO_NAMESPACE

#include "compiler/conversions.h"
#include "compiler/errors.h"
#include "compiler/mem.h"

void store(CompilerState *S, const Value *value, LLVMValueRef into) {
    if (value->type.kind == GRAMINA_TYPE_STRUCT) {
        LLVMBuildMemCpy(
            S->llvm_builder,
            into, 8,
            value->llvm, 8,
            LLVMSizeOf(value->type.llvm)
        );
    } else {
        LLVMBuildStore(S->llvm_builder, value->llvm, into);
    }
}

void try_load_inplace(CompilerState *S, Value *val) {
    if (val->class != GRAMINA_CLASS_ALLOCA) {
        return;
    }

    if (val->type.kind == GRAMINA_TYPE_STRUCT) {
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

    if (val->type.kind == GRAMINA_TYPE_STRUCT) {
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

    bool ptr_to_struct = operand.type.pointer_type->kind == GRAMINA_TYPE_STRUCT;
    if (ptr_to_struct) {
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

Value assign(CompilerState *S, Value *target, const Value *from) {
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

    Value loaded = try_load(S, from);

    if (!type_can_convert(S, &loaded.type, &target->type)) {
        err_implicit_conv(S, &loaded.type, &target->type);
        value_free(&loaded);
        return invalid_value();
    }

    convert_inplace(S, &loaded, &target->type);

    store(S, &loaded, ptr);
    // LLVMBuildStore(S->llvm_builder, value.llvm, ptr);

    loaded.class = GRAMINA_CLASS_RVALUE;

    return loaded;
}
