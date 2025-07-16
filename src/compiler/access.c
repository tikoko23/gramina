#define GRAMINA_NO_NAMESPACE

#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include "compiler/access.h"
#include "compiler/errors.h"
#include "compiler/mem.h"
#include "compiler/typedecl.h"
#include "compiler/value.h"

Value subscript(CompilerState *S, const Value *_scriptee, const Value *_scripter) {
    Value scriptee = try_load(S, _scriptee);
    Value scripter = try_load(S, _scripter);

    // TODO: operator overloading

    switch (scriptee.type.kind) {
    case GRAMINA_TYPE_ARRAY:
        if (scripter.type.kind != GRAMINA_TYPE_PRIMITIVE
         || !primitive_is_integral(scripter.type.primitive)) {
            StringView op = mk_sv_c("subscript");
            err_illegal_op(S, &scriptee.type, &scripter.type, &op);
            value_free(&scriptee);
            value_free(&scripter);
            break;
        }

        LLVMValueRef result = LLVMBuildInBoundsGEP2(
            S->llvm_builder,
            scriptee.type.llvm,
            scriptee.llvm,
            (LLVMValueRef[2]) {
                LLVMConstInt(LLVMInt32Type(), 0, false),
                scripter.llvm,
            }, 2, ""
        );

        LLVMValueRef loaded = kind_is_aggregate(scriptee.type.element_type->kind)
                            ? result
                            : LLVMBuildLoad2(S->llvm_builder, scriptee.type.element_type->llvm, result, "");

        Value ret = {
            .type = type_dup(scriptee.type.element_type),
            .llvm = loaded,
            .class = GRAMINA_CLASS_LVALUE,
            .lvalue_ptr = result,
        };

        value_free(&scriptee);
        value_free(&scripter);

        return ret;
    case GRAMINA_TYPE_POINTER:
        if (scriptee.type.pointer_type->kind == GRAMINA_TYPE_VOID) {
            StringView op = mk_sv_c("subscript");
            err_illegal_op(S, &scriptee.type, &scripter.type, &op);
            value_free(&scriptee);
            value_free(&scripter);
            break;
        }

        break;
    default: {
        StringView op = mk_sv_c("subscript");
        err_illegal_op(S, &scriptee.type, &scripter.type, &op);
        value_free(&scriptee);
        value_free(&scripter);
        break;
    }
    }

    value_free(&scriptee);
    value_free(&scripter);

    return invalid_value();
}

Value member(CompilerState *S, const Value *operand, const StringView *field_name) {
    Value lhs = try_load(S, operand);

    bool ptr_to_struct = lhs.type.kind == GRAMINA_TYPE_POINTER
                      && lhs.type.pointer_type->kind == GRAMINA_TYPE_STRUCT;

    // Pointer to struct and struct is pretty much the same thing in LLVM
    if (lhs.type.kind != GRAMINA_TYPE_STRUCT && !ptr_to_struct) {
        err_cannot_have_member(S, &lhs.type);
        value_free(&lhs);
        return invalid_value();
    }

    if (ptr_to_struct) {
        try_load_inplace(S, &lhs);
    }

    Type *struct_type = ptr_to_struct
                      ? lhs.type.pointer_type
                      : &lhs.type;

    StructField *field = hashmap_get(&struct_type->fields, *field_name);
    if (!field) {
        err_no_field(S, &lhs.type, field_name);
        value_free(&lhs);

        return invalid_value();
    }

    size_t offset = field->index;

    LLVMValueRef result = LLVMBuildInBoundsGEP2(
        S->llvm_builder,
        struct_type->llvm,
        lhs.llvm,
        (LLVMValueRef[2]){
            LLVMConstInt(LLVMInt32Type(), 0, 0), // Pick the struct the pointer is pointing to directly
            LLVMConstInt(LLVMInt32Type(), offset, 0),
        }, 2, ""
    );

    LLVMValueRef loaded = kind_is_aggregate(field->type.kind)
                        ? result
                        : LLVMBuildLoad2(S->llvm_builder, field->type.llvm, result, "");

    Value ret = {
        .type = type_dup(&field->type),
        .llvm = loaded,
        .class = GRAMINA_CLASS_LVALUE,
        .lvalue_ptr = result,
    };

    value_free(&lhs);
    return ret;
}

Value gramina_get_property(CompilerState *S, const Value *object, const StringView *prop_name) {
    Value lhs = try_load(S, object);

    switch (lhs.type.kind) {
    case GRAMINA_TYPE_SLICE:
        if (sv_cmp_c(prop_name, "length") == 0) {
            Value ret = {
                .llvm = LLVMBuildInBoundsGEP2(
                    S->llvm_builder,
                    lhs.type.llvm,
                    lhs.llvm,
                    (LLVMValueRef [2]) {
                        LLVMConstInt(LLVMInt32Type(), 0, false),
                        LLVMConstInt(LLVMInt32Type(), 1, false),
                    }, 2, ""
                ),
                .type = type_from_primitive(GRAMINA_PRIMITIVE_UINT),
                .class = GRAMINA_CLASS_RVALUE,
            };

            value_free(&lhs);
            ret.llvm = LLVMBuildLoad2(S->llvm_builder, ret.type.llvm, ret.llvm, "");
            return ret;
        } else if (sv_cmp_c(prop_name, "ptr") == 0) {
            Value ret = {
                .llvm = LLVMBuildInBoundsGEP2(
                    S->llvm_builder,
                    lhs.type.llvm,
                    lhs.llvm,
                    (LLVMValueRef [2]) {
                        LLVMConstInt(LLVMInt32Type(), 0, false),
                        LLVMConstInt(LLVMInt32Type(), 0, false),
                    }, 2, ""
                ),
                .type = mk_pointer_type(lhs.type.slice_type),
                .class = GRAMINA_CLASS_RVALUE,
            };

            value_free(&lhs);
            ret.llvm = LLVMBuildLoad2(S->llvm_builder, ret.type.llvm, ret.llvm, "");
            return ret;
        } else {
            err_no_getprop(S, &object->type, prop_name);

            value_free(&lhs);
            return invalid_value();
        }

        break;
    default:
        err_cannot_have_prop(S, &object->type);

        value_free(&lhs);
        return invalid_value();
    }

    return invalid_value();
}
