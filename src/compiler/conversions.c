#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#define GRAMINA_NO_NAMESPACE

#include "compiler/conversions.h"
#include "compiler/errors.h"
#include "compiler/mem.h"
#include "compiler/typedecl.h"
#include "compiler/value.h"

Value primitive_convert(CompilerState *S, const Value *_from, const Type *to) {
    Primitive p_from = _from->type.primitive;
    Primitive p_to = to->primitive;

    Value from = try_load(S, _from);

    if (p_from == p_to) {
        return from;
    }

    LLVMOpcode cast;

    if (primitive_is_integral(p_from) && primitive_is_integral(p_to)) {
        if (p_from > p_to) {
            cast = LLVMTrunc;
        } else {
            cast = primitive_is_unsigned(p_from)
                 ? LLVMZExt
                 : LLVMSExt;
        }
    } else if (!primitive_is_integral(p_from) && !primitive_is_integral(p_to)) {
        cast = p_from < p_to
             ? LLVMFPExt
             : LLVMFPTrunc;
    } else if (primitive_is_integral(p_from)) {
        cast = primitive_is_unsigned(p_from)
             ? LLVMUIToFP
             : LLVMSIToFP;
    } else {
        cast = primitive_is_unsigned(p_to)
             ? LLVMFPToUI
             : LLVMFPToSI;
    }

    LLVMValueRef ref = LLVMBuildCast(S->llvm_builder, cast, from.llvm, to->llvm, "");

    Value ret = {
        .llvm = ref,
        .type = type_dup(to),
        .class = from.class == GRAMINA_CLASS_CONSTEXPR
               ? GRAMINA_CLASS_CONSTEXPR
               : GRAMINA_CLASS_RVALUE
    };

    value_free(&from);

    return ret;
}

Value array_to_slice(CompilerState *S, const Value *from, const Type *slice_elem_type) {
    if (from->type.kind != GRAMINA_TYPE_ARRAY
     || !type_is_same(from->type.element_type, slice_elem_type)) {
        err_explicit_conv(S, &from->type, slice_elem_type);
        return invalid_value();
    }

    Type slice_type = mk_slice_type(slice_elem_type);

    LLVMValueRef slice_val = build_alloca(S, &slice_type, "tsl");
    LLVMValueRef ptr_val = LLVMBuildInBoundsGEP2(
        S->llvm_builder,
        slice_type.llvm,
        slice_val,
        (LLVMValueRef [2]) {
            LLVMConstInt(LLVMInt32Type(), 0, false),
            LLVMConstInt(LLVMInt32Type(), 0, false),
        }, 2, ""
    );

    LLVMValueRef len_val = LLVMBuildInBoundsGEP2(
        S->llvm_builder,
        slice_type.llvm,
        slice_val,
        (LLVMValueRef [2]) {
            LLVMConstInt(LLVMInt32Type(), 0, false),
            LLVMConstInt(LLVMInt32Type(), 1, false),
        }, 2, ""
    );

    Value length = mk_primitive_value(
        GRAMINA_PRIMITIVE_UINT,
        (PrimitiveInitialiser) { .u32 = from->type.length }
    );

    store(S, &length, len_val);
    value_free(&length);

    LLVMValueRef ptr = LLVMBuildInBoundsGEP2(
        S->llvm_builder,
        from->type.llvm,
        from->llvm,
        (LLVMValueRef [2]) {
            LLVMConstInt(LLVMInt32Type(), 0, false),
            LLVMConstInt(LLVMInt32Type(), 0, false),
        }, 2, ""
    );

    LLVMBuildStore(S->llvm_builder, ptr, ptr_val);

    return (Value) {
        .llvm = slice_val,
        .class = GRAMINA_CLASS_RVALUE,
        .type = slice_type,
    };
}

bool convert_inplace(CompilerState *S, Value *value, const Type *to) {
    if (type_is_same(&value->type, to)) {
        return true;
    }

    if (value->type.kind == GRAMINA_TYPE_PRIMITIVE && to->kind == GRAMINA_TYPE_PRIMITIVE) {
        Value new_value = primitive_convert(S, value, to);
        value_free(value);

        *value = new_value;
        return true;
    }

    if (value->type.kind == GRAMINA_TYPE_ARRAY && to->kind == GRAMINA_TYPE_SLICE) {
        Value new_value = array_to_slice(S, value, to->slice_type);
        value_free(value);

        *value = new_value;
        return value_is_valid(value); 
    }

    value_free(value);
    *value = invalid_value();
    return false;
}

Value convert(CompilerState *S, const Value *from, const Type *to) {
    if (type_is_same(&from->type, to)) {
        Value v = *from;
        v.type = type_dup(&from->type);
        return v;
    }

    if (from->type.kind == GRAMINA_TYPE_PRIMITIVE && to->kind == GRAMINA_TYPE_PRIMITIVE) {
        return primitive_convert(S, from, to);
    }

    if (from->type.kind == GRAMINA_TYPE_ARRAY && to->kind == GRAMINA_TYPE_SLICE) {
        return array_to_slice(S, from, to->slice_type);
    }

    return invalid_value();
}

CoercionResult coerce_primitives(CompilerState *S, const Value *left, const Value *right, Value *result_lhs, Value *result_rhs) {
    CoercionResult coerced = primitive_coercion(&left->type, &right->type);

    if (coerced.greater_type.kind == GRAMINA_TYPE_INVALID) {
        err_implicit_conv(S, &left->type, &right->type);

        *result_lhs = invalid_value();
        *result_rhs = invalid_value();
        return coerced;
    }

    *result_lhs = try_load(S, left);
    *result_rhs = try_load(S, right);

    if (left->type.primitive == right->type.primitive) {
        return coerced;
    } else if (coerced.is_right_promoted) {
        convert_inplace(S, result_rhs, &coerced.greater_type);
    } else {
        convert_inplace(S, result_lhs, &coerced.greater_type);
    }

    return coerced;
}

Value pointer_to_int(CompilerState *S, const Value *ptr) {
    Value from = try_load(S, ptr);

    Type int_type = type_from_primitive(GRAMINA_PRIMITIVE_ULONG);
    LLVMValueRef result = LLVMBuildPtrToInt(S->llvm_builder, from.llvm, int_type.llvm, "");

    Value ret = {
        .llvm = result,
        .type = int_type,
        .class = GRAMINA_CLASS_RVALUE,
    };

    value_free(&from);

    return ret;
}

Value cast(CompilerState *S, const Value *_from, const Type *to) {
    Value from = try_load(S, _from);

    if (type_is_same(&from.type, to)) {
        return from;
    }

    // TODO: explicit cast overloading

    if (from.type.kind == GRAMINA_TYPE_PRIMITIVE && to->kind == GRAMINA_TYPE_PRIMITIVE) {
        Value ret = primitive_convert(S, &from, to);
        value_free(&from);

        return ret;
    }

    if (from.type.kind == GRAMINA_TYPE_PRIMITIVE
     && to->kind == GRAMINA_TYPE_POINTER) {
        if (!primitive_is_integral(from.type.primitive)) {
            err_explicit_conv(S, &from.type, to);

            value_free(&from);

            return invalid_value();
        }

        LLVMValueRef result = LLVMBuildIntToPtr(S->llvm_builder, from.llvm, to->llvm, "");
        Value ret = {
            .llvm = result,
            .type = type_dup(to),
            .class = GRAMINA_CLASS_RVALUE,
        };

        value_free(&from);

        return ret;
    }

    if (from.type.kind == GRAMINA_TYPE_POINTER
     && to->kind == GRAMINA_TYPE_PRIMITIVE) {
        if (!primitive_is_integral(to->primitive)) {
            err_explicit_conv(S, &from.type, to);

            value_free(&from);

            return invalid_value();
        }

        LLVMValueRef result = LLVMBuildPtrToInt(S->llvm_builder, from.llvm, to->llvm, "");
        Value ret = {
            .llvm = result,
            .type = type_dup(to),
            .class = GRAMINA_CLASS_RVALUE,
        };

        value_free(&from);

        return ret;
    }

    return invalid_value();
}
