#define GRAMINA_NO_NAMESPACE

#include "common/log.h"

#include "compiler/errors.h"
#include "compiler/function.h"
#include "compiler/mem.h"
#include "compiler/stackops.h"
#include "compiler/statement.h"

GRAMINA_DECLARE_ARRAY(String, static);
GRAMINA_IMPLEMENT_ARRAY(String, static);

Value call(CompilerState *S, const Identifier *func, const Value *args, size_t n_params) {
    bool is_sret = func->type.return_type->kind == GRAMINA_TYPE_STRUCT;

    // In all cases, `llvm_args[0]` is reserved for sret
    LLVMValueRef llvm_args[n_params + 1];

    llvm_args[0] = is_sret
                 ? LLVMBuildAlloca(S->llvm_builder, func->type.return_type->llvm, "")
                 : NULL;

    for (size_t i = 1; i < n_params + 1; ++i) {
        if (args[i - 1].class == GRAMINA_CLASS_ALLOCA) {
            Value loaded = try_load(S, &args[i - 1]);
            llvm_args[i] = loaded.llvm;
            value_free(&loaded);
        } else {
            llvm_args[i] = args[i - 1].llvm;
        }
    }

    LLVMValueRef result = LLVMBuildCall2(
        S->llvm_builder,
        func->type.llvm,
        func->llvm,
        llvm_args + !is_sret, // Include the first param if sret
        n_params + is_sret,
        ""
    );

    Value ret = {
        .llvm = is_sret
              ? llvm_args[0]
              : result,
        .class = GRAMINA_CLASS_RVALUE,
        .type = type_dup(func->type.return_type),
    };

    return ret;
}

static Array(String) collect_param_names(AstNode *this) {
    Array(String) names = mk_array(String);

    if (!this) {
        return names;
    }

    AstNode *cur = this;
    do {
        if (cur->type == GRAMINA_AST_PARAM_LIST) {
            array_append(String, &names, str_dup(&cur->left->value.identifier));
        } else {
            array_append(String, &names, str_dup(&cur->value.identifier));
        }
    } while ((cur = cur->right));

    return names;
}

static bool check_tail_return(AstNode *this) {
    AstNode *cur = this;
    while ((cur = cur->right)) {
        if (cur->type == GRAMINA_AST_RETURN_STATEMENT) {
            return true;
        }
    }

    return false;
}

static void register_params(CompilerState *S, LLVMValueRef func, const Type *fn_type, bool sret, AstNode *this) {
    Array(String) param_names = collect_param_names(this->left->left);

    Scope *self_scope = CURRENT_SCOPE(S);

    array_foreach_ref(_GraminaType, i, type, fn_type->param_types) {
        LLVMValueRef temp = LLVMGetParam(func, i + sret);

        String *param_name = &param_names.items[i];

        LLVMValueRef llvm_param;
        if (type->kind != GRAMINA_TYPE_STRUCT) {
            char *cparam_name = str_to_cstr(param_name);

            LLVMValueRef allocated = LLVMBuildAlloca(S->llvm_builder, type->llvm, cparam_name);
            gramina_free(cparam_name);

            LLVMBuildStore(S->llvm_builder, temp, allocated);
            llvm_param = allocated;
        } else {
            LLVMAttributeRef byval_attr = LLVMCreateTypeAttribute(
                LLVMGetGlobalContext(),
                LLVMGetEnumAttributeKindForName("byval", 5),
                type->llvm
            );

            LLVMAddAttributeAtIndex(func, i + sret + 1, byval_attr); // 1 indexed

            llvm_param = temp;
        }

        Identifier *param = gramina_malloc(sizeof *param);
        *param = (Identifier) {
            .llvm = llvm_param,
            .type = type_dup(type),
            .kind = GRAMINA_IDENT_KIND_VAR,
        };

        hashmap_set(&self_scope->identifiers, str_as_view(param_name), param);

        str_free(param_name);
    }

    array_free(String, &param_names);
}

static bool validate_attributes(CompilerState *S, const Array(_GraminaSymAttr) *attribs, TokenPosition pos) {
    array_foreach_ref(_GraminaSymAttr, _, attrib, *attribs) {
        switch (attrib->kind) {
        case GRAMINA_ATTRIBUTE_METHOD:
            if (!attrib->string.data) {
                StringView name = mk_sv_c("method");
                err_no_attrib_arg(S, &name);
                S->error.pos = pos;
                return false;
            }

            break;
        default:
            break;
        }
    }

    return true;
}

void function_def(CompilerState *S, AstNode *this) {
    Type fn_type = type_from_ast_node(S, this->left);
    bool sret = fn_type.return_type->kind == GRAMINA_TYPE_STRUCT;

    StringView name = str_as_view(&this->value.identifier);

    if (hashmap_get(&CURRENT_SCOPE(S)->identifiers, name)) {
        err_redeclaration(S, &name);
        S->error.pos = this->left->pos;
        type_free(&fn_type);
        return;
    }

    char *name_cstr = sv_to_cstr(&name);
    LLVMValueRef func = LLVMAddFunction(S->llvm_module, name_cstr, fn_type.llvm);

    if (sret) {
        LLVMAttributeRef sret_attr = LLVMCreateTypeAttribute(
            LLVMGetGlobalContext(),
            LLVMGetEnumAttributeKindForName("sret", 4),
            fn_type.return_type->llvm
        );

        LLVMAddAttributeAtIndex(func, 1, sret_attr);
    }

    gramina_free(name_cstr);

    Identifier *fn_ident = gramina_malloc(sizeof *fn_ident);
    *fn_ident = (Identifier) {
        .kind = GRAMINA_IDENT_KIND_FUNC,
        .type = fn_type,
        .llvm = func,
        .attributes = this->value.attributes,
    };

    this->value.attributes = mk_array(_GraminaSymAttr); // We are essentially moving the array

    validate_attributes(S, &this->value.attributes, this->pos);

    Scope *parent_scope = CURRENT_SCOPE(S);
    hashmap_set(&parent_scope->identifiers, name, fn_ident);

    vlog_fmt("Registering function '{sv}'\n", &name);

    if (this->type == GRAMINA_AST_FUNCTION_DECLARATION) {
        return;
    }

    bool has_tail_return = check_tail_return(this);

    push_reflection(S, fn_type.return_type);

    LLVMBasicBlockRef body = LLVMAppendBasicBlock(func, "entry");
    LLVMPositionBuilderAtEnd(S->llvm_builder, body);

    push_scope(S);

    register_params(S, func, &fn_type, sret, this);

    block(S, func, this->right);
    pop_scope(S);

    if (!has_tail_return) {
        if (fn_type.return_type->kind == GRAMINA_TYPE_VOID) {
            LLVMBuildRetVoid(S->llvm_builder);
        } else {
            err_missing_ret(S, fn_type.return_type);
            S->error.pos = this->pos;
        }
    }

    pop_reflection(S);
}
