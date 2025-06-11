#define GRAMINA_NO_NAMESPACE

#include "common/log.h"

#include "compiler/errors.h"
#include "compiler/identifier.h"
#include "compiler/mem.h"
#include "compiler/scope.h"
#include "compiler/struct.h"

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

void struct_def(CompilerState *S, AstNode *this) {
    Type struct_type = type_from_ast_node(S, this);

    Identifier *ident = gramina_malloc(sizeof *ident);
    *ident = (Identifier) {
        .llvm = NULL,
        .type = struct_type,
        .kind = GRAMINA_IDENT_KIND_TYPE,
    };

    StringView name = str_as_view(&ident->type.struct_name);

    Scope *scope = CURRENT_SCOPE(S);
    hashmap_set(&scope->identifiers, name, ident);

    vlog_fmt("Registering struct '{sv}'\n", &name);
}
