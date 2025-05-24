#define GRAMINA_NO_NAMESPACE

#include <llvm-c/Core.h>

#include "common/str.h"
#include "common/hashmap.h"

#include "compiler/identifier.h"
#include "compiler/type.h"
#include "compiler/typedecl.h"

#include "parser/ast.h"

GRAMINA_IMPLEMENT_ARRAY(_GraminaType);

#define BUILTIN_PRIMITIVE(pr, _llvm) (Type) { .kind = GRAMINA_TYPE_PRIMITIVE, .primitive = GRAMINA_PRIMITIVE_ ## pr, .llvm = LLVM ## _llvm ## Type(), }

static void struct_field_free(void *field) {
    type_free(field);
    gramina_free(field);
}

static Type builtin_type(const StringView *i) {
    if (sv_cmp_c(i, "void") == GRAMINA_ZERO) {
        return (Type) {
            .kind = GRAMINA_TYPE_VOID,
            .llvm = LLVMVoidType(),
        };
    } else if (sv_cmp_c(i, "bool") == GRAMINA_ZERO) {
        return BUILTIN_PRIMITIVE(BOOL, Int1);
    } else if (sv_cmp_c(i, "byte") == GRAMINA_ZERO) {
        return BUILTIN_PRIMITIVE(BYTE, Int8);
    } else if (sv_cmp_c(i, "ubyte") == GRAMINA_ZERO) {
        return BUILTIN_PRIMITIVE(UBYTE, Int8);
    } else if (sv_cmp_c(i, "short") == GRAMINA_ZERO) {
        return BUILTIN_PRIMITIVE(SHORT, Int16);
    } else if (sv_cmp_c(i, "ushort") == GRAMINA_ZERO) {
        return BUILTIN_PRIMITIVE(USHORT, Int16);
    } else if (sv_cmp_c(i, "int") == GRAMINA_ZERO) {
        return BUILTIN_PRIMITIVE(INT, Int32);
    } else if (sv_cmp_c(i, "uint") == GRAMINA_ZERO) {
        return BUILTIN_PRIMITIVE(UINT, Int32);
    } else if (sv_cmp_c(i, "long") == GRAMINA_ZERO) {
        return BUILTIN_PRIMITIVE(LONG, Int64);
    } else if (sv_cmp_c(i, "ulong") == GRAMINA_ZERO) {
        return BUILTIN_PRIMITIVE(ULONG, Int64);
    } else if (sv_cmp_c(i, "float") == GRAMINA_ZERO) {
        return BUILTIN_PRIMITIVE(FLOAT, Float);
    } else if (sv_cmp_c(i, "double") == GRAMINA_ZERO) {
        return BUILTIN_PRIMITIVE(DOUBLE, Double);
    }

    return (Type) {
        .kind = GRAMINA_TYPE_INVALID,
    };
}

Type gramina_mk_pointer_type(const Type *pointed) {
    Type typ = {
        .kind = GRAMINA_TYPE_POINTER,
    };

    typ.pointer_type = gramina_malloc(sizeof *typ.pointer_type);
    *typ.pointer_type = type_dup(pointed);

    typ.llvm = LLVMPointerType(typ.pointer_type->llvm, GRAMINA_ZERO);

    return typ;
}

Type gramina_type_from_ast_node(CompilerState *S, const AstNode *this) {
    if (this == NULL) {
        return (Type) {
            .kind = GRAMINA_TYPE_VOID,
            .llvm = LLVMVoidType(),
        };
    }

    switch (this->type) {
    case GRAMINA_AST_REFLECT: {
        if (S->reflection.length == GRAMINA_ZERO) {
            if (S->has_error) {
                break;
            }

            S->has_error = true;
            S->error = (CompileError) {
                .pos = this->pos,
                .description = mk_str_c("illegal use of '$' out of context"),
            };

            break;
        }

        Reflection *reflection = array_last(_GraminaReflection, &S->reflection);
        if (S->reflection_depth > reflection->depth) {
            return type_dup(&reflection->type);
        } else {
            S->has_error = true;
            S->error = (CompileError) {
                .pos = this->pos,
                .description = mk_str_c("illegal use of '$' out of context"),
            };
        }

        break;
    }
    case GRAMINA_AST_IDENTIFIER: {
        StringView name = str_as_view(&this->value.identifier);
        Type builtin = builtin_type(&name);
        if (builtin.kind != GRAMINA_TYPE_INVALID) {
            return builtin;
        }

        Identifier *ident = resolve(S, &name);

        if (!ident || ident->kind != GRAMINA_IDENT_KIND_TYPE) {
            return (Type) {
                .kind = GRAMINA_TYPE_INVALID,
            };
        }

        return type_dup(&ident->type);
    }
    case GRAMINA_AST_FUNCTION_TYPE: {
        Type typ = {
            .kind = GRAMINA_TYPE_FUNCTION,
        };

        typ.return_type = gramina_malloc(sizeof *typ.return_type);
        *typ.return_type = type_from_ast_node(S, this->right);

        typ.param_types = mk_array(_GraminaType);

        if (this->left) {
            AstNode *current = this->left;

            do {
                if (current->type == GRAMINA_AST_PARAM_LIST) {
                    array_append(_GraminaType, &typ.param_types, type_from_ast_node(S, current->left->left));
                } else {
                    array_append(_GraminaType, &typ.param_types, type_from_ast_node(S, current->left));
                }
            } while ((current = current->right));
        }

        LLVMTypeRef params[typ.param_types.length];
        array_foreach_ref(_GraminaType, i, t, typ.param_types) {
            params[i] = t->llvm;
        }

        typ.llvm = LLVMFunctionType(typ.return_type->llvm, params, typ.param_types.length, false);

        return typ;
    }
    case GRAMINA_AST_TYPE_POINTER: {
        Type typ = {
            .kind = GRAMINA_TYPE_POINTER,
        };

        typ.pointer_type = gramina_malloc(sizeof *typ.pointer_type);
        *typ.pointer_type = type_from_ast_node(S, this->left);

        typ.llvm = LLVMPointerType(typ.pointer_type->llvm, GRAMINA_ZERO);

        return typ;
    }
    case GRAMINA_AST_TYPE_SLICE: {
        Type typ = {
            .kind = GRAMINA_TYPE_SLICE,
        };

        typ.slice_type = gramina_malloc(sizeof *typ.slice_type);
        *typ.slice_type = type_from_ast_node(S, this->left);

        return typ;
    }
    case GRAMINA_AST_STRUCT_DEF: {
        const AstNode *old_this = this;

        size_t field_count = GRAMINA_ZERO;
        while ((this = this->right)) {
            ++field_count;
        }

        this = old_this;

        Hashmap fields = mk_hashmap(
            field_count <= 16
                ? field_count
                : 16
        );

        fields.object_freer = struct_field_free;

        while ((this = this->right)) {
            StringView field_name = str_as_view(&this->left->value.identifier);
            Type *field_type = gramina_malloc(sizeof *field_type);
            *field_type = type_from_ast_node(S, this->left->left);

            hashmap_set(&fields, field_name, field_type);
        }

        this = old_this;

        return (Type) {
            .kind = GRAMINA_TYPE_STRUCT,
            .struct_name = str_dup(&this->left->value.identifier),
            .fields = fields,
        };
    }

    case GRAMINA_AST_VAL_BOOL:
        return type_from_primitive(GRAMINA_PRIMITIVE_BOOL);
    case GRAMINA_AST_VAL_CHAR:
        return type_from_primitive(GRAMINA_PRIMITIVE_UBYTE);

    case GRAMINA_AST_VAL_F32:
        return type_from_primitive(GRAMINA_PRIMITIVE_FLOAT);
    case GRAMINA_AST_VAL_F64:
        return type_from_primitive(GRAMINA_PRIMITIVE_DOUBLE);

    case GRAMINA_AST_VAL_I32:
        return type_from_primitive(GRAMINA_PRIMITIVE_INT);
    case GRAMINA_AST_VAL_U32:
        return type_from_primitive(GRAMINA_PRIMITIVE_UINT);

    case GRAMINA_AST_VAL_I64:
        return type_from_primitive(GRAMINA_PRIMITIVE_LONG);
    case GRAMINA_AST_VAL_U64:
        return type_from_primitive(GRAMINA_PRIMITIVE_ULONG);
    default:
        break;
    }

    return (Type) {
        .kind = GRAMINA_TYPE_INVALID,
    };
}

String gramina_type_to_str(const Type *this) {
    switch (this->kind) {
    case GRAMINA_TYPE_VOID:
        return mk_str_c("void");
    case GRAMINA_TYPE_PRIMITIVE:
        switch (this->primitive) {
        case GRAMINA_PRIMITIVE_BOOL:
            return mk_str_c("bool");
        case GRAMINA_PRIMITIVE_BYTE:
            return mk_str_c("byte");
        case GRAMINA_PRIMITIVE_UBYTE:
            return mk_str_c("ubyte");
        case GRAMINA_PRIMITIVE_SHORT:
            return mk_str_c("short");
        case GRAMINA_PRIMITIVE_USHORT:
            return mk_str_c("ushort");
        case GRAMINA_PRIMITIVE_INT:
            return mk_str_c("int");
        case GRAMINA_PRIMITIVE_UINT:
            return mk_str_c("uint");
        case GRAMINA_PRIMITIVE_LONG:
            return mk_str_c("long");
        case GRAMINA_PRIMITIVE_ULONG:
            return mk_str_c("ulong");
        case GRAMINA_PRIMITIVE_FLOAT:
            return mk_str_c("float");
        case GRAMINA_PRIMITIVE_DOUBLE:
            return mk_str_c("double");
        }
        break;
    case GRAMINA_TYPE_POINTER: {
        String pointed = type_to_str(this->pointer_type);
        str_append(&pointed, '&');

        return pointed;
    }
    case GRAMINA_TYPE_STRUCT:
        return str_dup(&this->struct_name);
    case GRAMINA_TYPE_SLICE: {
        String subtype = type_to_str(this->slice_type);
        str_cat_cstr(&subtype, "[]");

        return subtype;
    }
    default:
        break;
    }

    return mk_str_c("<err-type>");
}

Type gramina_type_from_primitive(Primitive p) {
    switch (p) {
    case GRAMINA_PRIMITIVE_BOOL:
        return BUILTIN_PRIMITIVE(BOOL, Int1);
    case GRAMINA_PRIMITIVE_BYTE:
        return BUILTIN_PRIMITIVE(BYTE, Int8);
    case GRAMINA_PRIMITIVE_UBYTE:
        return BUILTIN_PRIMITIVE(UBYTE, Int8);
    case GRAMINA_PRIMITIVE_SHORT:
        return BUILTIN_PRIMITIVE(SHORT, Int16);
    case GRAMINA_PRIMITIVE_USHORT:
        return BUILTIN_PRIMITIVE(USHORT, Int16);
    case GRAMINA_PRIMITIVE_INT:
        return BUILTIN_PRIMITIVE(INT, Int32);
    case GRAMINA_PRIMITIVE_UINT:
        return BUILTIN_PRIMITIVE(UINT, Int32);
    case GRAMINA_PRIMITIVE_LONG:
        return BUILTIN_PRIMITIVE(LONG, Int64);
    case GRAMINA_PRIMITIVE_ULONG:
        return BUILTIN_PRIMITIVE(ULONG, Int64);
    case GRAMINA_PRIMITIVE_FLOAT:
        return BUILTIN_PRIMITIVE(FLOAT, Float);
    case GRAMINA_PRIMITIVE_DOUBLE:
        return BUILTIN_PRIMITIVE(DOUBLE, Double);
    }
}

bool gramina_primitive_is_unsigned(Primitive this) {
    switch (this) {
    case GRAMINA_PRIMITIVE_BOOL:
    case GRAMINA_PRIMITIVE_UBYTE:
    case GRAMINA_PRIMITIVE_USHORT:
    case GRAMINA_PRIMITIVE_UINT:
    case GRAMINA_PRIMITIVE_ULONG:
        return true;
    default:
        return false;
    }
}

bool gramina_primitive_is_integral(Primitive this) {
    return this != GRAMINA_PRIMITIVE_FLOAT && this != GRAMINA_PRIMITIVE_DOUBLE;
}

bool gramina_type_is_same(const Type *a, const Type *b) {
    if (a->kind != b->kind) {
        return false;
    }

    switch (a->kind) {
    case GRAMINA_TYPE_INVALID:
        return false;
    case GRAMINA_TYPE_VOID:
        return true;
    case GRAMINA_TYPE_PRIMITIVE:
        return a->primitive == b->primitive;
    case GRAMINA_TYPE_POINTER:
        return type_is_same(a->pointer_type, b->pointer_type);
    case GRAMINA_TYPE_SLICE:
        return type_is_same(a->slice_type, b->slice_type);
    case GRAMINA_TYPE_STRUCT:
        if (str_cmp(&a->struct_name, &b->struct_name) != GRAMINA_ZERO) {
            return false;
        }

        hashmap_foreach(Type, name, atype, a->fields) {
            Type *btype = hashmap_get(&b->fields, name);
            if (!btype) {
                return false;
            }

            if (!type_is_same(atype, btype)) {
                return false;
            }
        }

        return true;
    default:
        return false;
    }
}

bool gramina_primitive_can_convert(Primitive a, Primitive b) {
    if (a == b) {
        return true;
    }

    return primitive_is_unsigned(a) == primitive_is_unsigned(b)
         ? b > a
         : false;
}

bool gramina_type_can_convert(const CompilerState *S, const Type *from, const Type *to) {
    if (type_is_same(from, to)) {
        return true;
    }

    if (from->kind == GRAMINA_TYPE_PRIMITIVE && to->kind == GRAMINA_TYPE_PRIMITIVE) {
        return primitive_can_convert(from->primitive, to->primitive);
    }

    return false;
}

Type gramina_decltype(const CompilerState *S, const AstNode *exp) {
    switch (exp->type) {
    case GRAMINA_AST_IDENTIFIER: {
        StringView name = str_as_view(&exp->value.identifier);
        Identifier *ident = resolve(S, &name);
        return type_dup(&ident->type);
    }
    case GRAMINA_AST_OP_ADDRESS_OF: {
        Type subtype = decltype(S, exp->left);

        Type ret = mk_pointer_type(&subtype);
        type_free(&subtype);

        return ret;
    }
    case GRAMINA_AST_OP_ADD:
    case GRAMINA_AST_OP_SUB:
    case GRAMINA_AST_OP_MUL:
    case GRAMINA_AST_OP_DIV:
    case GRAMINA_AST_OP_REM: {
        Type lhs = decltype(S, exp->left);
        Type rhs = decltype(S, exp->right);

        if (lhs.kind == GRAMINA_TYPE_PRIMITIVE && rhs.kind == GRAMINA_TYPE_PRIMITIVE) {
            CoercionResult res = primitive_coercion(&lhs, &rhs);
            type_free(&lhs);
            type_free(&rhs);

            return res.greater_type;
        }

        // TODO: operator overloading

        break;
    }
    default:
        break;
    }

    return (Type) {
        .kind = GRAMINA_TYPE_INVALID,
    };
}

CoercionResult gramina_primitive_coercion(const Type *p, const Type *q) {
    if (p->kind != GRAMINA_TYPE_PRIMITIVE
     || q->kind != GRAMINA_TYPE_PRIMITIVE
     || primitive_is_unsigned(p->primitive) != primitive_is_unsigned(q->primitive)) {
        return (CoercionResult) {
            .greater_type = {
                .kind = GRAMINA_TYPE_INVALID,
            },
            .is_right_promoted = false,
        };
    }

    Type greater = p->primitive > q->primitive
                 ? type_dup(p)
                 : type_dup(q);

    return (CoercionResult) {
        .greater_type = greater,
        .is_right_promoted = p->primitive > q->primitive
    };
}

Type gramina_type_dup(const Type *this) {
    if (this->kind == GRAMINA_TYPE_INVALID
     || this->kind == GRAMINA_TYPE_PRIMITIVE
     || this->kind == GRAMINA_TYPE_VOID) {
        return *this;
    }

    switch (this->kind) {
    case GRAMINA_TYPE_FUNCTION: {
        Type typ = {
            .kind = GRAMINA_TYPE_FUNCTION,
            .return_type = NULL,
            .param_types = mk_array(_GraminaType),
            .llvm = this->llvm,
        };

        if (this->return_type) {
            typ.return_type = gramina_malloc(sizeof *typ.return_type);
            *typ.return_type = type_dup(this->return_type);
        }

        if (this->param_types.length > GRAMINA_ZERO) {
            typ.param_types = mk_array_capacity(_GraminaType, this->param_types.length);
            array_foreach_ref(_GraminaType, i, param, this->param_types) {
                typ.param_types.items[i] = type_dup(param);
            }
        } else {
            typ.param_types = mk_array(_GraminaType);
        }

        return typ;
    }
    case GRAMINA_TYPE_POINTER: {
        Type typ = {
            .kind = GRAMINA_TYPE_POINTER,
            .llvm = this->llvm,
        };

        typ.pointer_type = gramina_malloc(sizeof *typ.pointer_type);
        *typ.pointer_type = type_dup(this->pointer_type);

        return typ;
    }
    case GRAMINA_TYPE_SLICE: {
        Type typ = {
            .kind = GRAMINA_TYPE_SLICE,
            .llvm = this->llvm,
        };

        typ.slice_type = gramina_malloc(sizeof *typ.slice_type);
        *typ.slice_type = type_dup(this->slice_type);

        return typ;
    }
    case GRAMINA_TYPE_STRUCT: {
        Type typ = {
            .kind = GRAMINA_TYPE_STRUCT,
            .struct_name = str_dup(&this->struct_name),
            .llvm = this->llvm,
        };

        typ.fields = mk_hashmap(this->fields.n_buckets);
        typ.fields.object_freer = this->fields.object_freer;

        hashmap_foreach(Type, key, value, this->fields) {
            Type *field_type = gramina_malloc(sizeof *field_type);
            *field_type = type_dup(value);

            hashmap_set(&typ.fields, key, field_type);
        }

        return typ;
    }
    default:
        return (Type) {
            .kind = GRAMINA_TYPE_INVALID,
        };
    }
}

void gramina_type_free(Type *this) {
    if (this == NULL) {
        return;
    }

    if (this->kind == GRAMINA_TYPE_INVALID
     || this->kind == GRAMINA_TYPE_PRIMITIVE
     || this->kind == GRAMINA_TYPE_VOID) {
        return;
    }

    switch (this->kind) {
    case GRAMINA_TYPE_FUNCTION:
        type_free(this->return_type);
        gramina_free(this->return_type);
        this->return_type = NULL;

        array_foreach_ref(_GraminaType, _, param, this->param_types) {
            type_free(param);
        }

        array_free(_GraminaType, &this->param_types);

        break;
    case GRAMINA_TYPE_POINTER:
        type_free(this->pointer_type);
        gramina_free(this->pointer_type);
        this->pointer_type = NULL;
        break;
    case GRAMINA_TYPE_SLICE:
        type_free(this->slice_type);
        gramina_free(this->slice_type);
        this->slice_type = NULL;
        break;
    case GRAMINA_TYPE_STRUCT:
        str_free(&this->struct_name);
        this->struct_name = mk_str();

        hashmap_free(&this->fields);
        this->fields = mk_hashmap(GRAMINA_ZERO);

        break;
    default:
        break;
    }
}
