#define GRAMINA_NO_NAMESPACE

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/PassBuilder.h>
#include <llvm-c/Types.h>

#include "common/hashmap.h"
#include "common/log.h"

#include "compiler/compiler.h"
#include "compiler/cstate.h"
#include "compiler/identifier.h"
#include "compiler/type.h"

#include "parser/ast.h"
#include "parser/attributes.h"

GRAMINA_IMPLEMENT_ARRAY(_GraminaReflection);

GRAMINA_DECLARE_ARRAY(String, static);
GRAMINA_IMPLEMENT_ARRAY(String, static);

#define CURRENT_SCOPE(S) (array_last(GraminaScope, &S->scopes))
#define REFLECT(index) (S->reflection.items + (index))

typedef enum {
    CLASS_INVALID,
    CLASS_ALLOCA,
    CLASS_LVALUE,
    CLASS_RVALUE,
    CLASS_CONSTEXPR,
} ValueClass;

typedef struct {
    union {
        LLVMValueRef lvalue_ptr;
    };

    ValueClass class;
    LLVMValueRef llvm;
    Type type;
} Value;

static bool value_is_valid(const Value *this) {
    return this->type.kind != GRAMINA_TYPE_INVALID
        && this->llvm != NULL
        && this->class != CLASS_INVALID;
}

static void value_free(Value *this) {
    type_free(&this->type);
}

static Value value_dup(const Value *this) {
    Value ret = *this;
    ret.type = type_dup(&this->type);

    return ret;
}

static Value invalid_value() {
    return (Value) {
        .type = {
            .kind = GRAMINA_TYPE_INVALID,
        },
        .llvm = NULL,
        .class = CLASS_INVALID,
    };
}

static int llvm_init(CompilerState *S) {
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    const char *module_name = "base"; // WIP
    S->llvm_module = LLVMModuleCreateWithName(module_name);
    S->llvm_builder = LLVMCreateBuilder();

    char *triple = LLVMGetDefaultTargetTriple(); // WIP
    LLVMSetTarget(S->llvm_module, triple);
    LLVMDisposeMessage(triple);

    return 0;
}

static int llvm_deinit(CompilerState *S) {
    LLVMDisposeBuilder(S->llvm_builder);

    return 0;
}

static StringView get_op(AstNodeType t) {
    switch (t) {
    case GRAMINA_AST_OP_ADD:
        return mk_sv_c("+");
    case GRAMINA_AST_OP_SUB:
        return mk_sv_c("-");
    case GRAMINA_AST_OP_MUL:
        return mk_sv_c("*");
    case GRAMINA_AST_OP_DIV:
        return mk_sv_c("/");
    case GRAMINA_AST_OP_REM:
        return mk_sv_c("%");
    case GRAMINA_AST_OP_LT:
        return mk_sv_c("<");
    case GRAMINA_AST_OP_GT:
        return mk_sv_c(">");
    case GRAMINA_AST_OP_LTE:
        return mk_sv_c("<=");
    case GRAMINA_AST_OP_GTE:
        return mk_sv_c(">=");
    case GRAMINA_AST_OP_EQUAL:
        return mk_sv_c("==");
    case GRAMINA_AST_OP_INEQUAL:
        return mk_sv_c("!=");
    default:
        return mk_sv_c("");
    }
}

static void putcs_err(CompilerState *S, const char *err, TokenPosition pos) {
    if (S->has_error) {
        return;
    }

    elog_fmt("Compilation: {cstr}\n", err);

    S->has_error = true;
    S->error = (CompileError) {
        .pos = pos,
        .description = mk_str_c(err),
    };
}

static void puts_err(CompilerState *S, String err, TokenPosition pos) {
    if (S->has_error) {
        str_free(&err);
        return;
    }

    elog_fmt("Compilation: {s}\n", &err);

    S->has_error = true;
    S->error = (CompileError) {
        .pos = pos,
        .description = err,
    };
}

static void err_implicit_conv(CompilerState *S, const Type *from, const Type *to, TokenPosition pos) {
    puts_err(S, str_cfmt("cannot implicitly convert '{so}' into '{so}'", type_to_str(from), type_to_str(to)), pos);
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

static void err_explicit_conv(CompilerState *S, const Type *from, const Type *to, TokenPosition pos) {
    puts_err(S, str_cfmt("no such conversion from '{so}' to '{so}' exists", type_to_str(from), type_to_str(to)), pos);
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

static void err_illegal_node(CompilerState *S, const AstNode *this) {
    StringView type = ast_node_type_to_str(this->type);
    puts_err(S, str_cfmt("illegal node '{sv}'", &type), this->pos);
    S->status = GRAMINA_COMPILE_ERR_ILLEGAL_NODE;
}

static void err_undeclared_ident(CompilerState *S, const StringView *name, TokenPosition pos) {
    puts_err(S, str_cfmt("use of undeclared identifier '{sv}'", name), pos);
    S->status = GRAMINA_COMPILE_ERR_UNDECLARED_IDENTIFIER;
}

static void err_redeclaration(CompilerState *S, const StringView *name, TokenPosition pos) {
    puts_err(S, str_cfmt("redeclaration of '{sv}'", name), pos);
    S->status = GRAMINA_COMPILE_ERR_REDECLARATION;
}

static void err_rvalue_assign(CompilerState *S, const Type *type, TokenPosition pos) {
    puts_err(S, str_cfmt("assigning to rvalue of type '{so}'", type_to_str(type)), pos);
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_VALUE_CLASS;
}

static void err_deref(CompilerState *S, const Type *type, TokenPosition pos) {
    puts_err(S, str_cfmt("use of '@' on incompatible type '{so}'", type_to_str(type)), pos);
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

static void err_illegal_op(CompilerState *S, const Type *l, const Type *r, const StringView *op, TokenPosition pos) {
    puts_err(S, str_cfmt(
        "illegal op '{sv}' on types '{so}' and '{so}'",
        op,
        type_to_str(l),
        type_to_str(r)
    ), pos);

    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

static void err_missing_ret(CompilerState *S, const Type *ret, TokenPosition pos) {
    puts_err(S, str_cfmt("type '{so}' must be returned on function tail", type_to_str(ret)), pos);
    S->status = GRAMINA_COMPILE_ERR_MISSING_RETURN;
}

static void err_cannot_call(CompilerState *S, const Type *type, TokenPosition pos) {
    puts_err(S, str_cfmt("cannot call value of type '{so}'", type_to_str(type)), pos);
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

static void err_cannot_have_member(CompilerState *S, const Type *type, TokenPosition pos) {
    puts_err(S, str_cfmt("value of type '{so}' cannot have member", type_to_str(type)), pos);
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

static void err_no_field(CompilerState *S, const Type *type, const StringView *field, TokenPosition pos) {
    const Type *indexed = type->kind == GRAMINA_TYPE_POINTER
                        ? type->pointer_type
                        : type;

    puts_err(S, str_cfmt("type '{so}' does not have '{sv}' field", type_to_str(indexed), field), pos);
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

static void err_insufficient_args(CompilerState *S, size_t wants, size_t got, TokenPosition pos) {
    puts_err(S, str_cfmt("function call expected {sz} argumen{cstr}, got {sz}", wants, wants == 1 ? "t" : "ts", got), pos);
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

static void err_excess_args(CompilerState *S, size_t wants, TokenPosition pos) {
    puts_err(S, str_cfmt("function call expected {sz} argumen{cstr}, got more", wants, wants == 1 ? "t" : "ts"), pos);
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

static void err_no_attrib_arg(CompilerState *S, const StringView *attrib_name, TokenPosition pos) {
    puts_err(S, str_cfmt("attribute '{sv}' needs an argument"), pos);
    S->status = GRAMINA_COMPILE_ERR_MISSING_ATTRIB_ARG;
}

static void dbg_print_val(const char *fmt, LLVMValueRef val) {
    char *val_str = LLVMPrintValueToString(val);
    printf(fmt, val_str);
    LLVMDisposeMessage(val_str);
}

static Value primitive_convert(CompilerState *S, const Value *from, const Type *to) {
    Primitive p_from = from->type.primitive;
    Primitive p_to = to->primitive;

    if (p_from == p_to) {
        return value_dup(from);
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

    LLVMValueRef ref = LLVMBuildCast(S->llvm_builder, cast, from->llvm, to->llvm, "");

    return (Value) {
        .llvm = ref,
        .type = type_dup(to),
        .class = from->class == CLASS_CONSTEXPR
               ? CLASS_CONSTEXPR
               : CLASS_RVALUE
    };
}

static void store(CompilerState *S, const Value *value, LLVMValueRef into) {
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

static void try_load_inplace(CompilerState *S, Value *val) {
    if (val->class != CLASS_ALLOCA) {
        return;
    }

    if (val->type.kind == GRAMINA_TYPE_STRUCT) {
        Value new_val = {
            .llvm = val->llvm,
            .class = CLASS_LVALUE,
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
        .class = CLASS_LVALUE,
        .type = type_dup(&val->type),
    };

    value_free(val);

    *val = new_val;
}

static Value try_load(CompilerState *S, const Value *val) {
    if (val->class != CLASS_ALLOCA) {
        return value_dup(val);
    }

    if (val->type.kind == GRAMINA_TYPE_STRUCT) {
        return (Value) {
            .llvm = val->llvm,
            .class = CLASS_LVALUE,
            .type = type_dup(&val->type),
            .lvalue_ptr = val->llvm,
        };
    }

    LLVMValueRef loaded = LLVMBuildLoad2(S->llvm_builder, val->type.llvm, val->llvm, "");

    return (Value) {
        .llvm = loaded,
        .lvalue_ptr = val->llvm,
        .class = CLASS_LVALUE,
        .type = type_dup(&val->type),
    };
}

static bool convert_inplace(CompilerState *S, Value *value, const Type *to) {
    if (type_is_same(&value->type, to)) {
        return true;
    }

    if (value->type.kind == GRAMINA_TYPE_PRIMITIVE && to->kind == GRAMINA_TYPE_PRIMITIVE) {
        Value new_value = primitive_convert(S, value, to);
        value_free(value);

        *value = new_value;
        return true;
    }

    value_free(value);
    *value = invalid_value();
    return false;
}

static Value convert(CompilerState *S, const Value *from, const Type *to) {
    if (from->type.kind == GRAMINA_TYPE_PRIMITIVE && to->kind == GRAMINA_TYPE_PRIMITIVE) {
        return primitive_convert(S, from, to);
    }

    return invalid_value();
}

static CoercionResult coerce_primitives(CompilerState *S, const Value *left, const Value *right, Value *lhs, Value *rhs, AstNode *this) {
    CoercionResult coerced = primitive_coercion(&left->type, &right->type);

    if (coerced.greater_type.kind == GRAMINA_TYPE_INVALID) {
        err_implicit_conv(S, &left->type, &right->type, this->pos);

        *lhs = invalid_value();
        *rhs = invalid_value();
        return coerced;
    }

    if (left->type.primitive == right->type.primitive) {
        *lhs = try_load(S, left);
        *rhs = try_load(S, right);
    } else if (coerced.is_right_promoted) {
        *lhs = try_load(S, left);
        *rhs = convert(S, right, &coerced.greater_type);
    } else {
        *lhs = convert(S, left, &coerced.greater_type);
        *rhs = try_load(S, right);
    }

    return coerced;
}

static Value primitive_arithmetic(CompilerState *S, AstNode *this, const Value *left, const Value *right) {
    Value lhs;
    Value rhs;

    CoercionResult coerced = coerce_primitives(S, left, right, &lhs, &rhs, this);
    if (S->has_error) {
        return invalid_value();
    }

    LLVMOpcode op;

    bool is_integer = primitive_is_integral(coerced.greater_type.primitive);
    bool signedness = !primitive_is_unsigned(coerced.greater_type.primitive);

    switch (this->type) {
    case GRAMINA_AST_OP_ADD:
        op = is_integer
           ? LLVMAdd
           : LLVMFAdd;
        break;
    case GRAMINA_AST_OP_SUB:
        op = is_integer
           ? LLVMSub
           : LLVMFSub;
        break;
    case GRAMINA_AST_OP_MUL:
        op = is_integer
           ? LLVMMul
           : LLVMFMul;
        break;
    case GRAMINA_AST_OP_DIV:
        if (is_integer) {
            op = signedness
               ? LLVMSDiv
               : LLVMUDiv;
        } else {
            op = LLVMFDiv;
        }
        break;
    case GRAMINA_AST_OP_REM:
        if (is_integer) {
            op = signedness
               ? LLVMSRem
               : LLVMURem;
        } else {
            op = LLVMFRem;
        }
        break;
    default:
        err_illegal_node(S, this);
        return invalid_value();
    }

    String tstr = type_to_str(&coerced.greater_type);
    str_free(&tstr);

    Value ret = {
        .type = coerced.greater_type,
        .class = left->class == CLASS_CONSTEXPR && right->class == CLASS_CONSTEXPR
               ? CLASS_CONSTEXPR
               : CLASS_RVALUE,
    };

    ret.llvm = LLVMBuildBinOp(S->llvm_builder, op, lhs.llvm, rhs.llvm, "");

    return ret;
}

static Scope *push_scope(CompilerState *S) {
    array_append(GraminaScope, &S->scopes, mk_scope());
    return array_last(GraminaScope, &S->scopes);
}

static void pop_scope(CompilerState *S) {
    scope_free(array_last(GraminaScope, &S->scopes));
    array_pop(GraminaScope, &S->scopes);
}

static void push_reflection(CompilerState *S, const Type *type) {
    array_append(_GraminaReflection, &S->reflection, ((Reflection) {
        .type = type_dup(type),
        .depth = S->reflection_depth
    }));
}

static void pop_reflection(CompilerState *S) {
    type_free(&array_last(_GraminaReflection, &S->reflection)->type);
    array_pop(_GraminaReflection, &S->reflection);
}

static Value pointer_to_int(CompilerState *S, const Value *ptr) {
    Value from = try_load(S, ptr);

    Type int_type = type_from_primitive(GRAMINA_PRIMITIVE_ULONG);
    LLVMValueRef result = LLVMBuildPtrToInt(S->llvm_builder, from.llvm, int_type.llvm, "");

    Value ret = {
        .llvm = result,
        .type = int_type,
        .class = CLASS_RVALUE,
    };

    value_free(&from);

    return ret;
}

static Value pointer_arithmetic(CompilerState *S, const Value *left, const Value *right, AstNode *this) {
    if (left->type.kind == right->type.kind && this->type != GRAMINA_AST_OP_SUB) {
        StringView op = get_op(this->type);
        err_illegal_op(S, &left->type, &right->type, &op, this->pos);

        return invalid_value();
    }

    if (this->type != GRAMINA_AST_OP_ADD
     && this->type != GRAMINA_AST_OP_SUB) {
        StringView op = get_op(this->type);
        err_illegal_op(S, &left->type, &right->type, &op, this->pos);

        return invalid_value();
    }

    if (left->type.kind == right->type.kind) {
        if (!type_is_same(&left->type, &right->type)) {
            StringView op = get_op(this->type);
            err_illegal_op(S, &left->type, &right->type, &op, this->pos);

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
            .class = CLASS_RVALUE,
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
    if (this->type == GRAMINA_AST_OP_SUB) {
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
        .class = CLASS_RVALUE,
    };

    return ret;
}

static Value expression(CompilerState *S, LLVMValueRef function, AstNode *this);
static Value arithmetic(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value left = expression(S, function, this->left);
    Value right = expression(S, function, this->right);

    if (left.type.kind == GRAMINA_TYPE_PRIMITIVE && right.type.kind == GRAMINA_TYPE_PRIMITIVE) {
        Value ret = primitive_arithmetic(S, this, &left, &right);

        value_free(&left);
        value_free(&right);

        return ret;
    }

    bool left_is_pointer_compatible = left.type.kind == GRAMINA_TYPE_POINTER;
    left_is_pointer_compatible |= left.type.kind == GRAMINA_TYPE_PRIMITIVE && primitive_is_integral(left.type.primitive);

    bool right_is_pointer_compatible = right.type.kind == GRAMINA_TYPE_POINTER;
    right_is_pointer_compatible |= right.type.kind == GRAMINA_TYPE_PRIMITIVE && primitive_is_integral(right.type.primitive);

    if (left_is_pointer_compatible && right_is_pointer_compatible) {
        try_load_inplace(S, &left);
        try_load_inplace(S, &right);

        Value ret = pointer_arithmetic(S, &left, &right, this);

        value_free(&left);
        value_free(&right);

        return ret;
    }

    return invalid_value();
}

static Value cast(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Type to = type_from_ast_node(S, this->left);
    if (S->has_error) {
        type_free(&to);
        return invalid_value();
    }

    Value from = expression(S, function, this->right);
    if (from.class == CLASS_ALLOCA) {
        Value loaded = try_load(S, &from);
        value_free(&from);
        from = loaded;
    }

    if (type_is_same(&from.type, &to)) {
        type_free(&to);
        return from;
    }

    // TODO: explicit cast overloading

    if (from.type.kind == GRAMINA_TYPE_PRIMITIVE && to.kind == GRAMINA_TYPE_PRIMITIVE) {
        Value ret = primitive_convert(S, &from, &to);
        type_free(&to);
        value_free(&from);

        return ret;
    }

    if (from.type.kind == GRAMINA_TYPE_PRIMITIVE
     && to.kind == GRAMINA_TYPE_POINTER) {
        if (!primitive_is_integral(from.type.primitive)) {
            err_explicit_conv(S, &from.type, &to, this->pos);

            type_free(&to);
            value_free(&from);

            return invalid_value();
        }

        LLVMValueRef result = LLVMBuildIntToPtr(S->llvm_builder, from.llvm, to.llvm, "");
        Value ret = {
            .llvm = result,
            .type = to,
            .class = CLASS_RVALUE,
        };

        value_free(&from);

        return ret;
    }

    if (from.type.kind == GRAMINA_TYPE_POINTER
     && to.kind == GRAMINA_TYPE_PRIMITIVE) {
        if (!primitive_is_integral(to.primitive)) {
            err_explicit_conv(S, &from.type, &to, this->pos);

            type_free(&to);
            value_free(&from);

            return invalid_value();
        }

        LLVMValueRef result = LLVMBuildPtrToInt(S->llvm_builder, from.llvm, to.llvm, "");
        Value ret = {
            .llvm = result,
            .type = to,
            .class = CLASS_RVALUE,
        };

        value_free(&from);

        return ret;
    }

    type_free(&to);
    return invalid_value();
}

static Value address_of(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value operand = expression(S, function, this->left);

    switch (operand.class) {
    case CLASS_ALLOCA: {
        Value ret = {
            .class = CLASS_RVALUE,
            .type = mk_pointer_type(&operand.type),
            .llvm = operand.llvm,
        };

        value_free(&operand);

        return ret;
    }
    case CLASS_LVALUE: {
        Value ret = {
            .class = CLASS_RVALUE,
            .type = mk_pointer_type(&operand.type),
            .llvm = operand.lvalue_ptr,
        };

        value_free(&operand);

        return ret;
    }
    default:
        puts_err(S, str_cfmt("taking address of rvalue of type '{so}'", type_to_str(&operand.type)), this->pos);
        return invalid_value();
    }
}

static Value assign(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value target = expression(S, function, this->left);

    push_reflection(S, &target.type);
    ++S->reflection_depth;

    Value value = expression(S, function, this->right);

    --S->reflection_depth;
    pop_reflection(S);

    LLVMValueRef ptr;
    switch (target.class) {
    case CLASS_ALLOCA:
        ptr = target.llvm;
        break;
    case CLASS_LVALUE:
        ptr = target.lvalue_ptr;
        break;
    default:
        err_rvalue_assign(S, &target.type, this->pos);
        value_free(&target);
        value_free(&value);
        return invalid_value();
    }

    try_load_inplace(S, &value);

    if (!type_can_convert(S, &value.type, &target.type)) {
        err_implicit_conv(S, &value.type, &target.type, this->pos);
        value_free(&target);
        value_free(&value);
        return invalid_value();
    }

    convert_inplace(S, &value, &target.type);

    store(S, &value, ptr);
    // LLVMBuildStore(S->llvm_builder, value.llvm, ptr);

    Value ret = value_dup(&value);
    ret.class = CLASS_RVALUE;

    value_free(&value);
    value_free(&target);

    return ret;
}

static Value deref(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value operand = expression(S, function, this->left);

    try_load_inplace(S, &operand);

    if (operand.type.kind != GRAMINA_TYPE_POINTER) {
        err_deref(S, &operand.type, this->pos);
        value_free(&operand);
        return invalid_value();
    }

    bool ptr_to_struct = operand.type.pointer_type->kind == GRAMINA_TYPE_STRUCT;
    if (ptr_to_struct) {
        Value ret = {
            .type = type_dup(operand.type.pointer_type),
            .llvm = operand.llvm,
            .class = CLASS_LVALUE,
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
        .class = CLASS_LVALUE,
    };

    value_free(&operand);
    return ret;
}

static Value primitive_comparison(CompilerState *S, const Value *left, const Value *right, AstNode *this) {
    Value lhs;
    Value rhs;

    CoercionResult coerced = coerce_primitives(S, left, right, &lhs, &rhs, this);
    if (!value_is_valid(&lhs) || !value_is_valid(&rhs) || coerced.greater_type.kind == GRAMINA_TYPE_INVALID) {
        return invalid_value();
    }

    bool signedness = !primitive_is_unsigned(coerced.greater_type.primitive);
    bool integral = primitive_is_integral(coerced.greater_type.primitive);

    LLVMValueRef result;

    if (integral) {
        LLVMIntPredicate op;
        switch (this->type) {
        case GRAMINA_AST_OP_EQUAL:
            op = LLVMIntEQ;
            break;
        case GRAMINA_AST_OP_INEQUAL:
            op = LLVMIntNE;
            break;
        case GRAMINA_AST_OP_GT:
            op = signedness
               ? LLVMIntSGT
               : LLVMIntUGT;
            break;
        case GRAMINA_AST_OP_GTE:
            op = signedness
               ? LLVMIntSGE
               : LLVMIntUGE;
            break;
        case GRAMINA_AST_OP_LT:
            op = signedness
               ? LLVMIntSLT
               : LLVMIntULT;
            break;
        case GRAMINA_AST_OP_LTE:
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
        switch (this->type) {
        case GRAMINA_AST_OP_EQUAL:
            op = LLVMRealOEQ;
            break;
        case GRAMINA_AST_OP_INEQUAL:
            op = LLVMRealONE;
            break;
        case GRAMINA_AST_OP_GT:
            op = LLVMRealOGT;
            break;
        case GRAMINA_AST_OP_GTE:
            op = LLVMRealOGE;
            break;
        case GRAMINA_AST_OP_LT:
            op = LLVMRealOLT;
            break;
        case GRAMINA_AST_OP_LTE:
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
        .class = CLASS_RVALUE,
    };

    value_free(&lhs);
    value_free(&rhs);

    type_free(&coerced.greater_type);

    return ret;
}

static Value comparison(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value left = expression(S, function, this->left);
    Value right = expression(S, function, this->right);

    if (left.type.kind == GRAMINA_TYPE_PRIMITIVE && right.type.kind == GRAMINA_TYPE_PRIMITIVE) {
        try_load_inplace(S, &left);
        try_load_inplace(S, &right);

        Value ret = primitive_comparison(S, &left, &right, this);

        value_free(&left);
        value_free(&right);

        return ret;
    }

    if (left.type.kind == GRAMINA_TYPE_POINTER && right.type.kind == GRAMINA_TYPE_POINTER) {
        if (!type_is_same(left.type.pointer_type, right.type.pointer_type)) {
            StringView op = get_op(this->type);
            err_illegal_op(S, &left.type, &right.type, &op, this->pos);

            value_free(&left);
            value_free(&right);

            return invalid_value();
        }

        LLVMIntPredicate op;
        switch (this->type) {
        case GRAMINA_AST_OP_EQUAL:
            op = LLVMIntEQ;
            break;
        case GRAMINA_AST_OP_INEQUAL:
            op = LLVMIntNE;
            break;
        case GRAMINA_AST_OP_GT:
            op = LLVMIntUGT;
            break;
        case GRAMINA_AST_OP_GTE:
            op = LLVMIntUGE;
            break;
        case GRAMINA_AST_OP_LT:
            op = LLVMIntULT;
            break;
        case GRAMINA_AST_OP_LTE:
            op = LLVMIntULE;
            break;
        default:
            break;
        }

        try_load_inplace(S, &left);
        try_load_inplace(S, &right);

        LLVMValueRef result = LLVMBuildICmp(S->llvm_builder, op, left.llvm, right.llvm, "");
        Value ret = {
            .llvm = result,
            .type = type_from_primitive(GRAMINA_PRIMITIVE_BOOL),
            .class = CLASS_RVALUE,
        };

        value_free(&left);
        value_free(&right);

        return ret;
    }

    return invalid_value();
}

static Value unary_arithmetic(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value operand = expression(S, function, this->left);

    try_load_inplace(S, &operand);

    if (operand.type.kind == GRAMINA_TYPE_PRIMITIVE) {
        switch (this->type) {
        case GRAMINA_AST_OP_UNARY_PLUS: {
            Value ret = {
                .type = type_dup(&operand.type),
                .llvm = operand.llvm,
                .class = CLASS_RVALUE,
            };

            value_free(&operand);

            return ret;
        }
        case GRAMINA_AST_OP_UNARY_MINUS: {
            if (primitive_is_unsigned(operand.type.primitive)) {
                puts_err(S, str_cfmt("use of unary minus '-' on unsigned type"), this->pos);
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
                .class = CLASS_RVALUE,
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

static Value binary_logic(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value lhs = expression(S, function, this->left);
    Value rhs = expression(S, function, this->right);

    Type bool_type = type_from_primitive(GRAMINA_PRIMITIVE_BOOL);

    if (!type_can_convert(S, &lhs.type, &bool_type)) {
        err_explicit_conv(S, &lhs.type, &bool_type, this->left->pos);

        type_free(&bool_type);
        value_free(&lhs);
        value_free(&rhs);

        return invalid_value();
    }

    if (!type_can_convert(S, &rhs.type, &bool_type)) {
        err_explicit_conv(S, &rhs.type, &bool_type, this->right->pos);

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

    switch (this->type) {
    case GRAMINA_AST_OP_LOGICAL_OR:
        result = LLVMBuildOr(S->llvm_builder, lhs.llvm, rhs.llvm, "");
        break;
    case GRAMINA_AST_OP_LOGICAL_XOR:
        result = LLVMBuildXor(S->llvm_builder, lhs.llvm, rhs.llvm, "");
        break;
    case GRAMINA_AST_OP_LOGICAL_AND:
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
        .class = CLASS_RVALUE,
    };

    value_free(&lhs);
    value_free(&rhs);

    return ret;
}

static bool convert_nodes_to_params(CompilerState *S, Identifier *func, Value *arguments, AstNode **params, size_t n_params) {
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
            err_implicit_conv(S, got, expected, param->pos);
            return false;
        }

        convert_inplace(S, arguments + i, expected);
        try_load_inplace(S, arguments + i);
    }

    return true;
}

static bool collect_params(CompilerState *S, AstNode *this, AstNode **params, size_t n_params) {
    AstNode *current = this->right;
    for (size_t i = 0; i < n_params; ++i) {
        if (!current) {
            err_insufficient_args(S, n_params, i, this->pos);
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
        err_excess_args(S, n_params, this->pos);
        return false;
    }

    return true;
}

static Value call(CompilerState *S, Identifier *func, Value *args, size_t n_params) {
    bool is_sret = func->type.return_type->kind == GRAMINA_TYPE_STRUCT;

    // In all cases, `llvm_args[0]` is reserved for sret
    LLVMValueRef llvm_args[n_params + 1];

    llvm_args[0] = is_sret
                 ? LLVMBuildAlloca(S->llvm_builder, func->type.return_type->llvm, "")
                 : NULL;

    for (size_t i = 1; i < n_params + 1; ++i) {
        llvm_args[i] = args[i - 1].llvm;
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
        .class = CLASS_RVALUE,
        .type = type_dup(func->type.return_type),
    };

    return ret;
}

static Value fn_call_expr(CompilerState *S, LLVMValueRef function, AstNode *this) {
    // TODO: operator overloading

    StringView func_name = str_as_view(&this->left->value.identifier);

    Identifier *func = resolve(S, &func_name);

    if (!func) {
        err_undeclared_ident(S, &func_name, this->left->pos);
        return invalid_value();
    }

    if (func->kind != GRAMINA_IDENT_KIND_FUNC) {
        err_cannot_call(S, &func->type, this->left->pos);
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

static Value member(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value lhs = expression(S, function, this->left);

    bool ptr_to_struct = lhs.type.kind == GRAMINA_TYPE_POINTER
                      && lhs.type.pointer_type->kind == GRAMINA_TYPE_STRUCT;

    // Pointer to struct and struct is pretty much the same thing in LLVM
    if (lhs.type.kind != GRAMINA_TYPE_STRUCT && !ptr_to_struct) {
        err_cannot_have_member(S, &lhs.type, this->pos);
        value_free(&lhs);
        return invalid_value();
    }

    if (ptr_to_struct) {
        try_load_inplace(S, &lhs);
    }

    Type *struct_type = ptr_to_struct
                      ? lhs.type.pointer_type
                      : &lhs.type;

    StringView rhs = str_as_view(&this->right->value.identifier);
    StructField *field = hashmap_get(&struct_type->fields, rhs);
    if (!field) {
        err_no_field(S, &lhs.type, &rhs, this->pos);
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

    LLVMValueRef loaded = field->type.kind == GRAMINA_TYPE_STRUCT
                        ? result
                        : LLVMBuildLoad2(S->llvm_builder, field->type.llvm, result, "");

    Value ret = {
        .type = type_dup(&field->type),
        .llvm = loaded,
        .class = CLASS_LVALUE,
        .lvalue_ptr = result,
    };

    value_free(&lhs);
    return ret;
}

static Value expression(CompilerState *S, LLVMValueRef function, AstNode *this) {
    switch (this->type) {
    case GRAMINA_AST_IDENTIFIER: {
        StringView name = str_as_view(&this->value.identifier);
        Identifier *ident = resolve(S, &name);

        if (!ident) {
            err_undeclared_ident(S, &name, this->pos);
            return invalid_value();
        }

        return (Value) {
            .type = type_dup(&ident->type),
            .class = CLASS_ALLOCA,
            .llvm = ident->llvm,
        };
    }
    case GRAMINA_AST_VAL_BOOL:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = CLASS_CONSTEXPR,
            .llvm = LLVMConstInt(LLVMInt1Type(), this->value.logical, false),
        };
    case GRAMINA_AST_VAL_CHAR:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = CLASS_CONSTEXPR,
            .llvm = LLVMConstInt(LLVMInt8Type(), this->value._char, false),
        };
    case GRAMINA_AST_VAL_I32:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = CLASS_CONSTEXPR,
            .llvm = LLVMConstInt(LLVMInt32Type(), this->value.i32, true),
        };
    case GRAMINA_AST_VAL_U32:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = CLASS_CONSTEXPR,
            .llvm = LLVMConstInt(LLVMInt32Type(), this->value.u32, false),
        };
    case GRAMINA_AST_VAL_I64:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = CLASS_CONSTEXPR,
            .llvm = LLVMConstInt(LLVMInt64Type(), this->value.i64, true),
        };
    case GRAMINA_AST_VAL_U64:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = CLASS_CONSTEXPR,
            .llvm = LLVMConstInt(LLVMInt64Type(), this->value.u64, false),
        };
    case GRAMINA_AST_VAL_F32:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = CLASS_CONSTEXPR,
            .llvm = LLVMConstReal(LLVMFloatType(), this->value.f32),
        };
    case GRAMINA_AST_VAL_F64:
        return (Value) {
            .type = type_from_ast_node(S, this),
            .class = CLASS_CONSTEXPR,
            .llvm = LLVMConstReal(LLVMDoubleType(), this->value.f64),
        };
    case GRAMINA_AST_OP_ADD:
    case GRAMINA_AST_OP_SUB:
    case GRAMINA_AST_OP_MUL:
    case GRAMINA_AST_OP_DIV:
    case GRAMINA_AST_OP_REM:
        return arithmetic(S, function, this);
    case GRAMINA_AST_OP_CAST:
        return cast(S, function, this);
    case GRAMINA_AST_OP_ADDRESS_OF:
        return address_of(S, function, this);
    case GRAMINA_AST_OP_DEREF:
        return deref(S, function, this);
    case GRAMINA_AST_OP_ASSIGN:
        return assign(S, function, this);
    case GRAMINA_AST_OP_EQUAL:
    case GRAMINA_AST_OP_INEQUAL:
    case GRAMINA_AST_OP_GT:
    case GRAMINA_AST_OP_GTE:
    case GRAMINA_AST_OP_LT:
    case GRAMINA_AST_OP_LTE:
        return comparison(S, function, this);
    case GRAMINA_AST_OP_UNARY_PLUS:
    case GRAMINA_AST_OP_UNARY_MINUS:
        return unary_arithmetic(S, function, this);
    case GRAMINA_AST_OP_LOGICAL_OR:
    case GRAMINA_AST_OP_LOGICAL_XOR:
    case GRAMINA_AST_OP_LOGICAL_AND:
        return binary_logic(S, function, this);
    case GRAMINA_AST_OP_CALL:
        if (this->left->type == GRAMINA_AST_IDENTIFIER) {
            return fn_call_expr(S, function, this);
        }

        return invalid_value();
    case GRAMINA_AST_OP_MEMBER:
        return member(S, function, this);
    default:
        err_illegal_node(S, this);
        return invalid_value();
    }
}

static void declaration(CompilerState *S, LLVMValueRef function, AstNode *this) {
    StringView name = str_as_view(&this->left->value.identifier);

    Scope *scope = CURRENT_SCOPE(S);
    if (hashmap_get(&scope->identifiers, name)) {
        err_redeclaration(S, &name, this->left->pos);
        return;
    }

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
            err_implicit_conv(S, &value.type, &ident_type, this->left->pos);
            type_free(&ident_type);
            type_free(&value.type);
            return;
        }

        convert_inplace(S, &value, &ident_type);
    }

    scope = CURRENT_SCOPE(S); // `gramina_realloc` may invalidate the previous pointer

    Identifier *ident = gramina_malloc(sizeof *ident);
    *ident = (Identifier) {
        .kind = GRAMINA_IDENT_KIND_VAR,
        .type = ident_type,
    };

    char *cname = sv_to_cstr(&name);
    ident->llvm = LLVMBuildAlloca(S->llvm_builder, ident->type.llvm, cname);
    gramina_free(cname);

    if (initialised) {
        store(S, &value, ident->llvm);
        // LLVMBuildStore(S->llvm_builder, value.llvm, ident->llvm);
    }

    hashmap_set(&scope->identifiers, name, ident);

    if (initialised) {
        value_free(&value);
    }
}

static void return_statement(CompilerState *S, LLVMValueRef function, AstNode *this) {
    ++S->reflection_depth;
    size_t reflection_index = S->reflection.length - 1;

    if (REFLECT(reflection_index)->type.kind == GRAMINA_TYPE_VOID) {
        if (this->left) {
            putcs_err(S, "return statement cannot have expression in function of type 'void'", this->left->pos);
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
        err_implicit_conv(S, &exp.type, &REFLECT(reflection_index)->type, this->left->pos);
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

    if (ret_type->kind == GRAMINA_TYPE_STRUCT) {
        LLVMBuildMemCpy(
            S->llvm_builder,
            LLVMGetParam(function, 0), 8,
            exp.llvm, 8,
            LLVMSizeOf(exp.type.llvm)
        );

        LLVMBuildRetVoid(S->llvm_builder);

    } else {
        LLVMBuildRet(S->llvm_builder, exp.llvm);
    }

    --S->reflection_depth;

    value_free(&exp);
}

static bool block(CompilerState *S, LLVMValueRef function, AstNode *this);
static void if_statement(CompilerState *S, LLVMValueRef function, AstNode *this) {
    Value condition = expression(S, function, this->left);
    if (S->has_error) {
        return;
    }

    Type bool_type = type_from_primitive(GRAMINA_PRIMITIVE_BOOL);
    if (!type_can_convert(S, &condition.type, &bool_type)) {
        err_implicit_conv(S, &condition.type, &bool_type, this->left->pos);

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

static void while_statement(CompilerState *S, LLVMValueRef function, AstNode *this) {
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
        err_implicit_conv(S, &condition.type, &bool_type, this->left->pos);

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

static void for_statement(CompilerState *S, LLVMValueRef function, AstNode *this) {
    push_scope(S);

    LLVMBasicBlockRef condition_block = LLVMAppendBasicBlock(function, "for_condition");
    LLVMBasicBlockRef expression_block = LLVMAppendBasicBlock(function, "for_expression");
    LLVMBasicBlockRef body_block = LLVMAppendBasicBlock(function, "for_body");
    LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(function, "for_exit");

    declaration(S, function, this->left->left);

    LLVMBuildBr(S->llvm_builder, condition_block);

    LLVMPositionBuilderAtEnd(S->llvm_builder, condition_block);
    Value predicate = expression(S, function, this->left->right->left);

    Type bool_type = type_from_primitive(GRAMINA_PRIMITIVE_BOOL);
    if (!type_can_convert(S, &predicate.type, &bool_type)) {
        err_implicit_conv(S, &predicate.type, &bool_type, this->left->right->left->pos);

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

static bool block(CompilerState *S, LLVMValueRef function, AstNode *this) {
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
            declaration(S, function, cur);
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
            err_illegal_node(S, cur);
            break;
        }
    } while ((cur = cur->right) && !S->status);

    return terminal;
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
                err_no_attrib_arg(S, &name, pos);
                return false;
            }

            break;
        default:
            break;
        }
    }

    return true;
}

static void function_def(CompilerState *S, AstNode *this) {
    Type fn_type = type_from_ast_node(S, this->left);
    bool sret = fn_type.return_type->kind == GRAMINA_TYPE_STRUCT;

    StringView name = str_as_view(&this->value.identifier);

    if (hashmap_get(&CURRENT_SCOPE(S)->identifiers, name)) {
        err_redeclaration(S, &name, this->left->pos);
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
            err_missing_ret(S, fn_type.return_type, this->pos);
        }
    }

    pop_reflection(S);
}

static void struct_def(CompilerState *S, AstNode *this) {
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

CompileResult gramina_compile(AstNode *root) {
    CompilerState S = {
        .has_error = false,
        .scopes = mk_array(GraminaScope),
        .reflection = mk_array(_GraminaReflection),
    };

    S.status = llvm_init(&S);
    if (S.status) {
        return (CompileResult) {
            .status = GRAMINA_COMPILE_ERR_LLVM,
            .error = {
                .description = mk_str_c("LLVM encountered an error during initialisation"),
                .pos = { 0, 0, 0 },
            },
        };
    }

    array_append(GraminaScope, &S.scopes, mk_scope());

    S.status = 0;
    AstNode *cur = root;
    do {
        switch (cur->left->type) {
        case GRAMINA_AST_FUNCTION_DEF:
        case GRAMINA_AST_FUNCTION_DECLARATION:
            function_def(&S, cur->left);
            break;
        case GRAMINA_AST_STRUCT_DEF:
            struct_def(&S, cur->left);
            break;
        default:
            break;
        }
    } while ((cur = cur->right) && !S.status);

    if (S.status) {
        array_foreach_ref(GraminaScope, _, s, S.scopes) {
            scope_free(s);
        }

        array_free(GraminaScope, &S.scopes);
        array_free(_GraminaReflection, &S.reflection);

        llvm_deinit(&S);
        LLVMDisposeModule(S.llvm_module);

        return (CompileResult) {
            .status = S.status,
            .error = S.error,
        };
    }

    gramina_assert(S.scopes.length == 1, "got: %zu", S.scopes.length);
    scope_free(array_last(GraminaScope, &S.scopes));
    array_free(GraminaScope, &S.scopes);
    array_free(_GraminaReflection, &S.reflection);

    S.status = llvm_deinit(&S);
    if (S.status) {
        LLVMDisposeModule(S.llvm_module);
        return (CompileResult) {
            .status = GRAMINA_COMPILE_ERR_LLVM,
            .error = {
                .description = mk_str_c("LLVM encountered an error during de-initialisation"),
                .pos = { 0, 0, 0 },
            },
        };
    }

    return (CompileResult) {
        .module = S.llvm_module,
        .status = GRAMINA_COMPILE_ERR_NONE,
    };
}


// s/GRAMINA_COMPILE_ERR_\(\w\+\).*/case GRAMINA_COMPILE_ERR_\1:\r        return mk_sv_c("\1");
StringView gramina_compile_error_code_to_str(CompileErrorCode e) {
    switch (e) {
    case GRAMINA_COMPILE_ERR_NONE:
        return mk_sv_c("NONE");
    case GRAMINA_COMPILE_ERR_LLVM:
        return mk_sv_c("LLVM");
    case GRAMINA_COMPILE_ERR_DUPLICATE_IDENTIFIER:
        return mk_sv_c("DUPLICATE_IDENTIFIER");
    case GRAMINA_COMPILE_ERR_UNDECLARED_IDENTIFIER:
        return mk_sv_c("UNDECLARED_IDENTIFIER");
    case GRAMINA_COMPILE_ERR_REDECLARATION:
        return mk_sv_c("REDECLARATION");
    case GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE:
        return mk_sv_c("INCOMPATIBLE_TYPE");
    case GRAMINA_COMPILE_ERR_ILLEGAL_NODE:
        return mk_sv_c("ILLEGAL_NODE");
    case GRAMINA_COMPILE_ERR_MISSING_RETURN:
        return mk_sv_c("MISSING_RETURN");
    case GRAMINA_COMPILE_ERR_INCOMPATIBLE_VALUE_CLASS:
        return mk_sv_c("INCOMPATIBLE_VALUE_CLASS");
    case GRAMINA_COMPILE_ERR_MISSING_ATTRIB_ARG:
        return mk_sv_c("MISSING_ATTRIB_ARG");
    default:
        return mk_sv_c("UNKNOWN");
    }
}
