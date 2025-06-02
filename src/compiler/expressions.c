#define GRAMINA_NO_NAMESPACE

#include "compiler/arithmetic.h"
#include "compiler/conversions.h"
#include "compiler/errors.h"
#include "compiler/expressions.h"
#include "compiler/function.h"
#include "compiler/identifier.h"
#include "compiler/logic.h"
#include "compiler/mem.h"
#include "compiler/stackops.h"
#include "compiler/struct.h"

Value expression(CompilerState *S, LLVMValueRef function, AstNode *this) {
    switch (this->type) {
    case GRAMINA_AST_IDENTIFIER: {
        StringView name = str_as_view(&this->value.identifier);
        Identifier *ident = resolve(S, &name);

        if (!ident) {
            err_undeclared_ident(S, &name);
            S->error.pos = this->pos;
            return invalid_value();
        }

        return (Value) {
            .type = type_dup(&ident->type),
            .class = GRAMINA_CLASS_ALLOCA,
            .llvm = ident->llvm,
        };
    }
    case GRAMINA_AST_VAL_BOOL:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = GRAMINA_CLASS_CONSTEXPR,
            .llvm = LLVMConstInt(LLVMInt1Type(), this->value.logical, false),
        };
    case GRAMINA_AST_VAL_CHAR:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = GRAMINA_CLASS_CONSTEXPR,
            .llvm = LLVMConstInt(LLVMInt8Type(), this->value._char, false),
        };
    case GRAMINA_AST_VAL_I32:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = GRAMINA_CLASS_CONSTEXPR,
            .llvm = LLVMConstInt(LLVMInt32Type(), this->value.i32, true),
        };
    case GRAMINA_AST_VAL_U32:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = GRAMINA_CLASS_CONSTEXPR,
            .llvm = LLVMConstInt(LLVMInt32Type(), this->value.u32, false),
        };
    case GRAMINA_AST_VAL_I64:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = GRAMINA_CLASS_CONSTEXPR,
            .llvm = LLVMConstInt(LLVMInt64Type(), this->value.i64, true),
        };
    case GRAMINA_AST_VAL_U64:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = GRAMINA_CLASS_CONSTEXPR,
            .llvm = LLVMConstInt(LLVMInt64Type(), this->value.u64, false),
        };
    case GRAMINA_AST_VAL_F32:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = GRAMINA_CLASS_CONSTEXPR,
            .llvm = LLVMConstReal(LLVMFloatType(), this->value.f32),
        };
    case GRAMINA_AST_VAL_F64:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = GRAMINA_CLASS_CONSTEXPR,
            .llvm = LLVMConstReal(LLVMDoubleType(), this->value.f64),
        };
    case GRAMINA_AST_OP_ADD:
    case GRAMINA_AST_OP_SUB:
    case GRAMINA_AST_OP_MUL:
    case GRAMINA_AST_OP_DIV:
    case GRAMINA_AST_OP_REM:
        return arithmetic_expr(S, function, this);
    case GRAMINA_AST_OP_CAST:
        return cast_expr(S, function, this);
    case GRAMINA_AST_OP_ADDRESS_OF:
        return address_of_expr(S, function, this);
    case GRAMINA_AST_OP_DEREF:
        return deref_expr(S, function, this);
    case GRAMINA_AST_OP_ASSIGN:
        return assign_expr(S, function, this);
    case GRAMINA_AST_OP_EQUAL:
    case GRAMINA_AST_OP_INEQUAL:
    case GRAMINA_AST_OP_GT:
    case GRAMINA_AST_OP_GTE:
    case GRAMINA_AST_OP_LT:
    case GRAMINA_AST_OP_LTE:
        return comparison_expr(S, function, this);
    case GRAMINA_AST_OP_UNARY_PLUS:
    case GRAMINA_AST_OP_UNARY_MINUS:
        return unary_arithmetic_expr(S, function, this);
    case GRAMINA_AST_OP_LOGICAL_OR:
    case GRAMINA_AST_OP_LOGICAL_XOR:
    case GRAMINA_AST_OP_LOGICAL_AND:
        return binary_logic_expr(S, function, this);
    case GRAMINA_AST_OP_CALL:
        if (this->left->type == GRAMINA_AST_IDENTIFIER) {
            return fn_call_expr(S, function, this);
        }

        return invalid_value();
    case GRAMINA_AST_OP_MEMBER:
        return member_expr(S, function, this);
    default:
        err_illegal_node(S, this->type);
        S->error.pos = this->pos;
        return invalid_value();
    }
}

Value arithmetic_expr(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value left = expression(S, function, this->left);
    Value right = expression(S, function, this->right);

    Value ret = arithmetic(S, &left, &right, get_op_from_ast_node(this));

    value_free(&left);
    value_free(&right);

    if (!value_is_valid(&ret)) {
        S->error.pos = this->pos;
    }

    return ret;
}

Value cast_expr(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Type to = type_from_ast_node(S, this->left);
    if (S->has_error) {
        type_free(&to);
        return invalid_value();
    }

    Value from = expression(S, function, this->right);
    Value ret = cast(S, &from, &to);

    value_free(&from);
    type_free(&to);

    if (!value_is_valid(&ret)) {
        S->error.pos = this->pos;
    }

    return ret;
}

Value address_of_expr(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value operand = expression(S, function, this->left);
    Value ret = address_of(S, &operand);

    value_free(&operand);

    if (!value_is_valid(&ret)) {
        S->error.pos = this->pos;
    }

    return ret;
}


Value assign_expr(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value target = expression(S, function, this->left);

    push_reflection(S, &target.type);
    ++S->reflection_depth;

    Value value = expression(S, function, this->right);

    --S->reflection_depth;
    pop_reflection(S);

    Value ret = assign(S, &target, &value);

    value_free(&value);
    value_free(&target);

    if (!value_is_valid(&ret)) {
        S->error.pos = this->pos;
    }

    return ret;
}


Value deref_expr(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value operand = expression(S, function, this->left);
    Value ret = deref(S, &operand);

    value_free(&operand);

    if (!value_is_valid(&ret)) {
        S->error.pos = this->pos;
    }

    return ret;
}

Value comparison_expr(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value left = expression(S, function, this->left);
    Value right = expression(S, function, this->right);
    Value ret = comparison(S, &left, &right, get_op_from_ast_node(this));

    value_free(&left);
    value_free(&right);

    if (!value_is_valid(&ret)) {
        S->error.pos = this->pos;
    }

    return ret;
}

Value unary_arithmetic_expr(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value operand = expression(S, function, this->left);
    Value ret = unary_arithmetic(S, &operand, get_op_from_ast_node(this));

    value_free(&operand);

    if (!value_is_valid(&ret)) {
        S->error.pos = this->pos;
    }

    return ret;
}

Value binary_logic_expr(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value lhs = expression(S, function, this->left);
    Value rhs = expression(S, function, this->right);

    Value ret = binary_logic(S, &lhs, &rhs, get_op_from_ast_node(this));

    value_free(&lhs);
    value_free(&rhs);

    if (!value_is_valid(&ret)) {
        S->error.pos = this->pos;
    }

    return ret;
}

bool collect_params(CompilerState *S, AstNode *this, AstNode **params, size_t n_params) {
    AstNode *current = this->right;
    for (size_t i = 0; i < n_params; ++i) {
        if (!current) {
            err_insufficient_args(S, n_params, i);
            S->error.pos = this->pos;
            return false;
        }

        if (current->type == GRAMINA_AST_EXPRESSION_LIST) {
            params[i] = current->left;
        } else {
            params[i] = current;
        }

        current = current->right;
    }

    if (current && current->parent->type == GRAMINA_AST_EXPRESSION_LIST
     || n_params == 0 && this->right != NULL) {
        err_excess_args(S, n_params);
        S->error.pos = this->pos;
        return false;
    }

    return true;
}

bool convert_nodes_to_params(CompilerState *S, Identifier *func, Value *arguments, AstNode **params, size_t n_params) {
    const Type *fn_type = &func->type;
    LLVMValueRef llvm_handle = func->llvm;

    for (size_t i = 0; i < n_params; ++i) {
        Type *expected = fn_type->param_types.items + i;

        AstNode *param = params[i];

        push_reflection(S, expected);
        ++S->reflection_depth;

        arguments[i] = expression(S, llvm_handle, param);

        --S->reflection_depth;
        pop_reflection(S);

        Type *got = &arguments[i].type;

        if (!type_can_convert(S, got, expected)) {
            err_implicit_conv(S, got, expected);
            S->error.pos = param->pos;
            return false;
        }

        convert_inplace(S, arguments + i, expected);
        try_load_inplace(S, arguments + i);
    }

    return true;
}

Value fn_call_expr(CompilerState *S, LLVMValueRef function, AstNode *this) {
    // TODO: operator overloading

    StringView func_name = str_as_view(&this->left->value.identifier);

    Identifier *func = resolve(S, &func_name);

    if (!func) {
        err_undeclared_ident(S, &func_name);
        S->error.pos = this->left->pos;
        return invalid_value();
    }

    if (func->kind != GRAMINA_IDENT_KIND_FUNC) {
        err_cannot_call(S, &func->type);
        S->error.pos = this->left->pos;
        return invalid_value();
    }

    size_t n_params = func->type.param_types.length;
    size_t params_capacity = n_params + 1;
    AstNode *params[params_capacity]; // The array should not have a size of 0

    if (!collect_params(S, this, params, n_params)) {
        return invalid_value();
    }

    Value arguments[params_capacity];
    if (!convert_nodes_to_params(S, func, arguments, params, n_params)) {
        return invalid_value();
    }

    Value ret = call(S, func, arguments, n_params);

    for (size_t i = 0; i < n_params; ++i) {
        value_free(arguments + i);
    }

    return ret;
}

Value member_expr(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value lhs = expression(S, function, this->left);
    StringView rhs = str_as_view(&this->right->value.identifier);
    Value ret = member(S, &lhs, &rhs);

    value_free(&lhs);

    if (!value_is_valid(&ret)) {
        S->error.pos = this->pos;
    }

    return ret;
}
