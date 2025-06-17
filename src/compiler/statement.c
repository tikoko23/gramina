#define GRAMINA_NO_NAMESPACE

#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include "compiler/conversions.h"
#include "compiler/errors.h"
#include "compiler/expressions.h"
#include "compiler/identifier.h"
#include "compiler/mem.h"
#include "compiler/stackops.h"
#include "compiler/statement.h"

Identifier *declaration(CompilerState *S, const StringView *name, const Type *type, const Value *init) {
    Scope *scope = CURRENT_SCOPE(S);
    if (hashmap_get(&scope->identifiers, *name)) {
        err_redeclaration(S, name);
        return NULL;
    }

    if (init && !init_respects_constness(S, &init->type, type)) {
        err_discard_const(S, &init->type, type);
        return NULL;
    }

    Identifier *ident = gramina_malloc(sizeof *ident);
    *ident = (Identifier) {
        .kind = GRAMINA_IDENT_KIND_VAR,
        .type = type_dup(type),
    };

    char *cname = sv_to_cstr(name);
    ident->llvm = build_alloca(S, &ident->type, cname);
    gramina_free(cname);

    if (init) {
        store(S, init, ident->llvm);
    }

    hashmap_set(&scope->identifiers, *name, ident);

    return ident;
}

void declaration_statement(CompilerState *S, LLVMValueRef function, AstNode *this) {
    StringView name = str_as_view(&this->left->value.identifier);

    Type ident_type = type_from_ast_node(S, this->left->left);
    if (S->has_error) {
        return;
    }

    bool initialised = this->left->right != NULL;
    Value value = {};

    if (initialised) {
        push_reflection(S, &ident_type);
        ++S->reflection_depth;

        value = expression(S, function, this->left->right);
        try_load_inplace(S, &value);

        --S->reflection_depth;
        pop_reflection(S);

        if (!value_is_valid(&value)) {
            type_free(&ident_type);
            return;
        }

        if (!type_can_convert(S, &value.type, &ident_type)) {
            err_implicit_conv(S, &value.type, &ident_type);
            S->error.pos = this->left->pos;
            type_free(&ident_type);
            type_free(&value.type);
            return;
        }

        convert_inplace(S, &value, &ident_type);
    }

    declaration(S, &name, &ident_type, initialised ? &value : NULL);
    if (S->has_error) {
        S->error.pos = this->pos;
    }

    if (initialised) {
        value_free(&value);
    }

    type_free(&ident_type);
}

void return_statement(CompilerState *S, LLVMValueRef function, AstNode *this) {
    ++S->reflection_depth;
    size_t reflection_index = S->reflection.length - 1;

    if (REFLECT(reflection_index)->type.kind == GRAMINA_TYPE_VOID) {
        if (this->left) {
            putcs_err(S, "return statement cannot have expression in function of type 'void'");
            S->error.pos = this->left->pos;
            S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
            return;
        }

        LLVMBuildRetVoid(S->llvm_builder);
        return;
    }

    Value exp = expression(S, function, this->left);
    if (S->has_error) {
        return;
    }

    if (!type_can_convert(S, &exp.type, &REFLECT(reflection_index)->type)) {
        err_implicit_conv(S, &exp.type, &REFLECT(reflection_index)->type);
        S->error.pos = this->left->pos;
        value_free(&exp);
        return;
    }

    if (!type_is_same(&exp.type, &REFLECT(reflection_index)->type)) {
        Value ret = convert(S, &exp, &REFLECT(reflection_index)->type);
        value_free(&exp);
        exp = ret;
    }

    Type *ret_type = &REFLECT(reflection_index)->type;
    try_load_inplace(S, &exp);
    convert_inplace(S, &exp, ret_type);

    switch (ret_type->kind) {
    case GRAMINA_TYPE_STRUCT:
        LLVMBuildMemCpy(
            S->llvm_builder,
            LLVMGetParam(function, 0), 8,
            exp.llvm, 8,
            LLVMSizeOf(exp.type.llvm)
        );

        LLVMBuildRetVoid(S->llvm_builder);
        break;

    case GRAMINA_TYPE_ARRAY: {
        LLVMBuildMemCpy(
            S->llvm_builder,
            LLVMGetParam(function, 0), 16, // This too is a temporary hack for the
                                           // same reasons mentioned in mem.c
            exp.llvm, 16,
            LLVMSizeOf(exp.type.llvm)
        );

        LLVMBuildRetVoid(S->llvm_builder);
        break;
    }
    default:
        LLVMBuildRet(S->llvm_builder, exp.llvm);
        break;
    }

    --S->reflection_depth;

    value_free(&exp);
}

void if_statement(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value condition = expression(S, function, this->left);
    if (S->has_error) {
        return;
    }

    Type bool_type = type_from_primitive(GRAMINA_PRIMITIVE_BOOL);
    if (!type_can_convert(S, &condition.type, &bool_type)) {
        err_implicit_conv(S, &condition.type, &bool_type);
        S->error.pos = this->left->pos;

        value_free(&condition);
        type_free(&bool_type);

        return;
    }

    try_load_inplace(S, &condition);

    AstNode *else_clause = this->right->right;
    LLVMBasicBlockRef else_block = !else_clause
                                 ? NULL
                                 : LLVMAppendBasicBlock(function, "else");

    LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(function, "then");
    LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(function, "merge");

    if (else_clause) {
        LLVMBuildCondBr(S->llvm_builder, condition.llvm, then_block, else_block);
    } else {
        LLVMBuildCondBr(S->llvm_builder, condition.llvm, then_block, merge_block);
    }

    LLVMPositionBuilderAtEnd(S->llvm_builder, then_block);
    push_scope(S);

    bool then_terminated = block(S, function, this->right->left);
    if (!then_terminated) {
        LLVMBuildBr(S->llvm_builder, merge_block);
    }

    pop_scope(S);

    if (else_clause) {
        LLVMPositionBuilderAtEnd(S->llvm_builder, else_block);
        push_scope(S);

        bool else_terminated = block(S, function, else_clause->left);
        if (!else_terminated) {
            LLVMBuildBr(S->llvm_builder, merge_block);
        }

        pop_scope(S);
    }

    LLVMPositionBuilderAtEnd(S->llvm_builder, merge_block);
}

void while_statement(CompilerState *S, LLVMValueRef function, AstNode *this) {
    LLVMBasicBlockRef condition_block = LLVMAppendBasicBlock(function, "while_condition");
    LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(function, "while_exit");
    LLVMBasicBlockRef body_block = LLVMAppendBasicBlock(function, "while_body");

    LLVMBuildBr(S->llvm_builder, condition_block);
    LLVMPositionBuilderAtEnd(S->llvm_builder, condition_block);

    Value condition = expression(S, function, this->left);
    if (S->has_error) {
        return;
    }

    Type bool_type = type_from_primitive(GRAMINA_PRIMITIVE_BOOL);
    if (!type_can_convert(S, &condition.type, &bool_type)) {
        err_implicit_conv(S, &condition.type, &bool_type);
        S->error.pos = this->left->pos;

        value_free(&condition);
        type_free(&bool_type);

        return;
    }

    try_load_inplace(S, &condition);

    LLVMBuildCondBr(S->llvm_builder, condition.llvm, body_block, exit_block);

    LLVMPositionBuilderAtEnd(S->llvm_builder, body_block);
    push_scope(S);

    bool body_terminated = block(S, function, this->right);
    if (!body_terminated) {
        LLVMBuildBr(S->llvm_builder, condition_block);
    }

    pop_scope(S);

    LLVMPositionBuilderAtEnd(S->llvm_builder, exit_block);
}

void for_statement(CompilerState *S, LLVMValueRef function, AstNode *this) {
    push_scope(S);

    LLVMBasicBlockRef condition_block = LLVMAppendBasicBlock(function, "for_condition");
    LLVMBasicBlockRef expression_block = LLVMAppendBasicBlock(function, "for_expression");
    LLVMBasicBlockRef body_block = LLVMAppendBasicBlock(function, "for_body");
    LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(function, "for_exit");

    declaration_statement(S, function, this->left->left);

    LLVMValueRef inst = LLVMBuildBr(S->llvm_builder, condition_block);

    LLVMPositionBuilderAtEnd(S->llvm_builder, condition_block);
    Value predicate = expression(S, function, this->left->right->left);

    Type bool_type = type_from_primitive(GRAMINA_PRIMITIVE_BOOL);
    if (!type_can_convert(S, &predicate.type, &bool_type)) {
        err_implicit_conv(S, &predicate.type, &bool_type);
        S->error.pos = this->left->right->left->pos;

        value_free(&predicate);
        type_free(&bool_type);

        return;
    }

    try_load_inplace(S, &predicate);

    LLVMBuildCondBr(S->llvm_builder, predicate.llvm, body_block, exit_block);

    LLVMPositionBuilderAtEnd(S->llvm_builder, body_block);
    bool body_terminated = block(S, function, this->right);
    if (!body_terminated) {
        LLVMBuildBr(S->llvm_builder, expression_block);
    }

    LLVMPositionBuilderAtEnd(S->llvm_builder, expression_block);
    Value result = expression(S, function, this->left->right->right);
    LLVMBuildBr(S->llvm_builder, condition_block);

    LLVMPositionBuilderAtEnd(S->llvm_builder, exit_block);

    value_free(&result);
    value_free(&predicate);

    pop_scope(S);
}

bool block(CompilerState *S, LLVMValueRef function, AstNode *this) {
    if (!this) {
        return false;
    }

    bool terminal = false;

    AstNode *cur = this;
    do {
        terminal = false;
        StringView type = ast_node_type_to_str(cur->type);
        switch (cur->type) {
        case GRAMINA_AST_DECLARATION_STATEMENT:
            declaration_statement(S, function, cur);
            break;
        case GRAMINA_AST_EXPRESSION_STATEMENT: {
            Value result = expression(S, function, cur->left);
            value_free(&result);
            break;
        }
        case GRAMINA_AST_RETURN_STATEMENT:
            return_statement(S, function, cur);
            terminal = true;
            break;
        case GRAMINA_AST_CONTROL_FLOW:
            switch (cur->left->type) {
            case GRAMINA_AST_IF_STATEMENT:
                if_statement(S, function, cur->left);
                break;
            case GRAMINA_AST_WHILE_STATEMENT:
                while_statement(S, function, cur->left);
                break;
            case GRAMINA_AST_FOR_STATEMENT:
                for_statement(S, function, cur->left);
                break;
            default:
                break;
            }
            break;
        default:
            err_illegal_node(S, cur->type);
            break;
        }
    } while ((cur = cur->right) && !S->status);

    return terminal;
}
