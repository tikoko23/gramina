#define GRAMINA_NO_NAMESPACE

#include "common/log.h"

#include "compiler/errors.h"
#include "compiler/type.h"

void putcs_err(CompilerState *S, const char *err) {
    if (S->has_error) {
        return;
    }

    elog_fmt("Compilation: {cstr}\n", err);

    S->has_error = true;
    S->error = (CompileError) {
        .description = mk_str_c(err),
    };
}

void puts_err(CompilerState *S, String err) {
    if (S->has_error) {
        str_free(&err);
        return;
    }

    elog_fmt("Compilation: {s}\n", &err);

    S->has_error = true;
    S->error = (CompileError) {
        .description = err,
    };
}

void err_implicit_conv(CompilerState *S, const Type *from, const Type *to) {
    puts_err(S, str_cfmt("cannot implicitly convert '{so}' into '{so}'", type_to_str(from), type_to_str(to)));
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

void err_explicit_conv(CompilerState *S, const Type *from, const Type *to) {
    puts_err(S, str_cfmt("no such conversion from '{so}' to '{so}' exists", type_to_str(from), type_to_str(to)));
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

void err_illegal_node(CompilerState *S, AstNodeType type) {
    StringView type_str = ast_node_type_to_str(type);
    puts_err(S, str_cfmt("illegal node '{sv}'", &type_str));
    S->status = GRAMINA_COMPILE_ERR_ILLEGAL_NODE;
}

void err_undeclared_ident(CompilerState *S, const StringView *name) {
    puts_err(S, str_cfmt("use of undeclared identifier '{sv}'", name));
    S->status = GRAMINA_COMPILE_ERR_UNDECLARED_IDENTIFIER;
}

void err_redeclaration(CompilerState *S, const StringView *name) {
    puts_err(S, str_cfmt("redeclaration of '{sv}'", name));
    S->status = GRAMINA_COMPILE_ERR_REDECLARATION;
}

void err_rvalue_assign(CompilerState *S, const Type *type) {
    puts_err(S, str_cfmt("assigning to rvalue of type '{so}'", type_to_str(type)));
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_VALUE_CLASS;
}

void err_deref(CompilerState *S, const Type *type) {
    puts_err(S, str_cfmt("use of '@' on incompatible type '{so}'", type_to_str(type)));
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

void err_illegal_op(CompilerState *S, const Type *l, const Type *r, const StringView *op) {
    puts_err(S, str_cfmt(
        "illegal op '{sv}' on types '{so}' and '{so}'",
        op,
        type_to_str(l),
        type_to_str(r)
    ));

    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

void err_missing_ret(CompilerState *S, const Type *ret) {
    puts_err(S, str_cfmt("type '{so}' must be returned on function tail", type_to_str(ret)));
    S->status = GRAMINA_COMPILE_ERR_MISSING_RETURN;
}

void err_cannot_call(CompilerState *S, const Type *type) {
    puts_err(S, str_cfmt("cannot call value of type '{so}'", type_to_str(type)));
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

void err_cannot_have_member(CompilerState *S, const Type *type) {
    puts_err(S, str_cfmt("value of type '{so}' cannot have member", type_to_str(type)));
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

void err_cannot_have_prop(CompilerState *S, const Type *type) {
    puts_err(S, str_cfmt("value of type '{so}' cannot have property", type_to_str(type)));
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

void err_no_field(CompilerState *S, const Type *type, const StringView *field) {
    const Type *indexed = type->kind == GRAMINA_TYPE_POINTER
                        ? type->pointer_type
                        : type;

    puts_err(S, str_cfmt("type '{so}' does not have '{sv}' field", type_to_str(indexed), field));
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

void err_no_getprop(CompilerState *S, const Type *type, const StringView *prop) {
    puts_err(S, str_cfmt("type '{so}' does not have get-able property '{sv}'", type_to_str(type), prop));
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

void err_insufficient_args(CompilerState *S, size_t wants, size_t got) {
    puts_err(S, str_cfmt("function call expected {sz} argumen{cstr}, got {sz}", wants, wants == 1 ? "t" : "ts", got));
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

void err_excess_args(CompilerState *S, size_t wants) {
    puts_err(S, str_cfmt("function call expected {sz} argumen{cstr}, got more", wants, wants == 1 ? "t" : "ts"));
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

void err_no_attrib_arg(CompilerState *S, const StringView *attrib_name) {
    puts_err(S, str_cfmt("attribute '{sv}' needs an argument"));
    S->status = GRAMINA_COMPILE_ERR_MISSING_ATTRIB_ARG;
}

void err_const_assign(CompilerState *S, const Type *type) {
    puts_err(S, str_cfmt("assigning to constant value of type '{so}'", type_to_str(type)));
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

void err_discard_const(CompilerState *S, const Type *from, const Type *to) {
    puts_err(S, str_cfmt("converting from '{so}' into '{so}' discards const qualifiers", type_to_str(from), type_to_str(to)));
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}

void err_bad_type(CompilerState *S, const Type *type) {
    puts_err(S, str_cfmt("bad type '{so}'", type_to_str(type)));
    S->status = GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE;
}
