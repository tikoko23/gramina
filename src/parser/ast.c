#define GRAMINA_NO_NAMESPACE

#include <stdint.h>
#include <string.h>

#include "common/array.h"
#include "common/str.h"

#include "parser/ast.h"
#include "parser/attributes.h"

#undef bool

typedef _Bool bool;

GRAMINA_DECLARE_ARRAY(bool, static);
GRAMINA_IMPLEMENT_ARRAY(bool, static);

AstNode *gramina_mk_ast_node(AstNode *parent) {
    AstNode *this = gramina_malloc(sizeof *this);
    if (this == NULL) {
        return this;
    }

    if (parent != NULL) {
        if (parent->left == NULL) {
            parent->left = this;
        } else {
            gramina_assert(parent->right == NULL);
            parent->right = this;
        }
    }

    this->parent = parent;
    this->left = NULL;
    this->right = NULL;
    memset(&this->value, 0, (sizeof this->value));

    return this;
}

AstNode *gramina_mk_ast_node_lr(AstNode *parent, AstNode *l, AstNode *r) {
    AstNode *this = mk_ast_node(parent);
    if (this == NULL) {
        return this;
    }

    this->left = l;
    this->right = r;
    memset(&this->value, 0, (sizeof this->value));

    if (l) {
        l->parent = this;
    }

    if (r) {
        r->parent = this;
    }

    return this;
}

void gramina_ast_node_free(AstNode *this) {
    if (this == NULL) {
        return;
    }

    switch (this->type) {
    case GRAMINA_AST_IDENTIFIER:
    case GRAMINA_AST_FUNCTION_DEF:
    case GRAMINA_AST_FUNCTION_DECLARATION:
        str_free(&this->value.identifier);

        array_foreach_ref(_GraminaSymAttr, _, attrib, this->value.attributes) {
            symattr_free(attrib);
        }

        array_free(_GraminaSymAttr, &this->value.attributes);
        break;
    case GRAMINA_AST_STRUCT_DEF:
        array_foreach_ref(_GraminaSymAttr, _, attrib, this->value.attributes) {
            symattr_free(attrib);
        }

        array_free(_GraminaSymAttr, &this->value.attributes);
        break;
    default:
        break;
    }

    ast_node_free(this->left);
    ast_node_free(this->right);

    this->left = NULL;
    this->right = NULL;
    this->parent = NULL;

    gramina_free(this);
}

void gramina_ast_node_child_l(AstNode *this, AstNode *new_child) {
    this->left = new_child;

    if (new_child) {
        new_child->parent = this;
    }
}

void gramina_ast_node_child_r(AstNode *this, AstNode *new_child) {
    this->right = new_child;

    if (new_child) {
        new_child->parent = this;
    }
}

SymbolAttribute *gramina_ast_node_get_symattr(const AstNode *this, SymbolAttributeKind kind) {
    switch (this->type) {
    case GRAMINA_AST_IDENTIFIER:
    case GRAMINA_AST_FUNCTION_DEF:
    case GRAMINA_AST_FUNCTION_DECLARATION:
    case GRAMINA_AST_STRUCT_DEF:
        break;
    default:
        return NULL;
    }

    array_foreach_ref(_GraminaSymAttr, _, attr, this->value.attributes) {
        if (attr->kind == kind) {
            return attr;
        }
    }

    return NULL;
}

static String ast_node_stringify(const AstNode *this) {
    String out = mk_str();

    switch (this->type) {
    case GRAMINA_AST_VAL_F32:
        str_cat_cfmt(&out, "value: {f32}", this->value.f32);
        break;
    case GRAMINA_AST_VAL_F64:
        str_cat_cfmt(&out, "value: {f64}", this->value.f64);
        break;
    case GRAMINA_AST_VAL_I32:
        str_cat_cfmt(&out, "value: {i32}", this->value.i32);
        break;
    case GRAMINA_AST_VAL_U32:
        str_cat_cfmt(&out, "value: {u32}", this->value.u32);
        break;
    case GRAMINA_AST_VAL_I64:
        str_cat_cfmt(&out, "value: {i64}", this->value.i64);
        break;
    case GRAMINA_AST_VAL_U64:
        str_cat_cfmt(&out, "value: {u64}", this->value.u64);
        break;
    case GRAMINA_AST_VAL_BOOL:
        str_cat_cfmt(&out, "value: {cstr}", this->value.logical ? "true" : "false");
        break;
    case GRAMINA_AST_IDENTIFIER:
    case GRAMINA_AST_FUNCTION_DEF:
    case GRAMINA_AST_FUNCTION_DECLARATION:
        str_cat_cfmt(&out, "name: {s}, {sz} attribute(s)", &this->value.identifier, this->value.attributes.length);
        break;
    default:
        break;
    }

    return out;
}

static int __gramina_ast_print(const AstNode *this, Stream *printer, Array(bool) *strokes, size_t sibling_count, bool is_left) {
    enum {
        EMPTY,
        DOWNSTROKE,
        SPLIT,
        TERMINAL,
    };

    static const char *sections[] = {
        "    ",
        " ┃  ",
        " ┣━╸",
        " ┗━╸",
    };

    gramina_assert(stream_is_writable(printer));
    if (!stream_is_writable(printer)) {
        return EOF;
    }

    if (this == NULL && sibling_count == 0) {
        return 0;
    }

    String out = mk_str();

    if (strokes->length > 0) {
        Slice(bool) sl = array_slice(bool, strokes, 0, strokes->length - 1);

        slice_foreach(bool, i, b, sl) {
            StringView sv = b
                          ? mk_sv_c(sections[DOWNSTROKE])
                          : mk_sv_c(sections[EMPTY]);

            str_cat_sv(&out, &sv);
        }

        StringView connector = is_left
                             ? mk_sv_c(sections[TERMINAL])
                             : mk_sv_c(sections[SPLIT]);

        str_cat_sv(&out, &connector);
    }

    if (this == NULL) {
        str_cat_cstr(&out, "(nil)\n");
        int status = stream_write_str(printer, &out);
        str_free(&out);
        return status;
    }

    StringView type = ast_node_type_to_str(this->type);
    StringView fmt = mk_sv_c("self: %p, parent: %p, left: %p, right: %p");
    String self = ast_node_stringify(this);
    if (self.length) {
        str_cat_cfmt(
            &out,
            "{sv} ({u64}:{u64}) [{s}]\n",
            &type,
            (uint64_t)this->pos.column,
            (uint64_t)this->pos.line,
            &self
        );
    } else {
        str_cat_cfmt(
            &out,
            "{sv} ({u64}:{u64})\n",
            &type,
            (uint64_t)this->pos.column,
            (uint64_t)this->pos.line
        );
    }

    str_free(&self);

    int status = stream_write_str(printer, &out);
    str_free(&out);

    if (status) {
        return status;
    }

    size_t child_count = !!this->left + !!this->right;

    array_append(bool, strokes, true);
    int r1 = __gramina_ast_print(this->left, printer, strokes, child_count, false);

    *array_last(bool, strokes) = false;
    int r2 = __gramina_ast_print(this->right, printer, strokes, child_count, true);

    array_pop(bool, strokes);

    if (r1) {
        return r1;
    }

    if (r2) {
        return r2;
    }

    return 0;
}

int gramina_ast_print(const AstNode *this, struct gramina_stream *printer) {
    Array(bool) strokes = mk_array(bool);
    int status = __gramina_ast_print(this, printer, &strokes, 1, false);

    array_free(bool, &strokes);

    return status;
}

// s/GRAMINA_AST_\(\w\+\).*,/case GRAMINA_AST_\1:\r    \treturn mk_sv_c("\1");
StringView gramina_ast_node_type_to_str(AstNodeType e) {
    switch (e) {
    case GRAMINA_AST_INVALID:
        return mk_sv_c("INVALID");

    case GRAMINA_AST_VAL_CHAR:
        return mk_sv_c("VAL_CHAR");
    case GRAMINA_AST_VAL_STRING:
        return mk_sv_c("VAL_STRING");

    case GRAMINA_AST_VAL_F32:
        return mk_sv_c("VAL_F32");
    case GRAMINA_AST_VAL_F64:
        return mk_sv_c("VAL_F64");

    case GRAMINA_AST_VAL_I32:
        return mk_sv_c("VAL_I32");
    case GRAMINA_AST_VAL_U32:
        return mk_sv_c("VAL_U32");

    case GRAMINA_AST_VAL_I64:
        return mk_sv_c("VAL_I64");
    case GRAMINA_AST_VAL_U64:
        return mk_sv_c("VAL_U64");

    case GRAMINA_AST_VAL_BOOL:
        return mk_sv_c("VAL_BOOL");

    case GRAMINA_AST_TYPE_SLICE:
        return mk_sv_c("TYPE_SLICE");
    case GRAMINA_AST_TYPE_ARRAY:
        return mk_sv_c("TYPE_ARRAY");
    case GRAMINA_AST_TYPE_POINTER:
        return mk_sv_c("TYPE_POINTER");

    case GRAMINA_AST_CONTROL_FLOW:
        return mk_sv_c("CONTROL_FLOW");

    case GRAMINA_AST_GLOBAL_STATEMENT:
        return mk_sv_c("GLOBAL_STATEMENT");
    case GRAMINA_AST_EXPRESSION_STATEMENT:
        return mk_sv_c("EXPRESSION_STATEMENT");
    case GRAMINA_AST_DECLARATION_STATEMENT:
        return mk_sv_c("DECLARATION_STATEMENT");
    case GRAMINA_AST_RETURN_STATEMENT:
        return mk_sv_c("RETURN_STATEMENT");
    case GRAMINA_AST_IF_STATEMENT:
        return mk_sv_c("IF_STATEMENT");
    case GRAMINA_AST_FOR_STATEMENT:
    	return mk_sv_c("FOR_STATEMENT");
    case GRAMINA_AST_WHILE_STATEMENT:
    	return mk_sv_c("WHILE_STATEMENT");

    case GRAMINA_AST_ELSE_CLAUSE:
        return mk_sv_c("ELSE_CLAUSE");
    case GRAMINA_AST_ELSE_IF_CLAUSE:
        return mk_sv_c("ELSE_IF_CLAUSE");

    case GRAMINA_AST_PARAM_LIST:
        return mk_sv_c("PARAM_LIST");
    case GRAMINA_AST_EXPRESSION_LIST:
        return mk_sv_c("EXPRESSION_LIST");

    case GRAMINA_AST_SLICE:
        return mk_sv_c("SLICE");
    case GRAMINA_AST_REFLECT:
        return mk_sv_c("REFLECT");

    case GRAMINA_AST_FUNCTION_TYPE:
        return mk_sv_c("FUNCTION_TYPE");
    case GRAMINA_AST_FUNCTION_DEF:
        return mk_sv_c("FUNCTION_DEF");
    case GRAMINA_AST_FUNCTION_DECLARATION:
        return mk_sv_c("FUNCTION_DECLARATION");
    case GRAMINA_AST_FUNCTION_DESCRIPTION:
        return mk_sv_c("FUNCTION_DESCRIPTION");
    case GRAMINA_AST_IDENTIFIER:
        return mk_sv_c("IDENTIFIER");

    case GRAMINA_AST_STRUCT_DEF:
        return mk_sv_c("STRUCT_DEF");
    case GRAMINA_AST_STRUCT_FIELD:
        return mk_sv_c("STRUCT_FIELD");

    case GRAMINA_AST_OP_CAST:
        return mk_sv_c("OP_CAST");

    case GRAMINA_AST_OP_STATIC_MEMBER:
        return mk_sv_c("OP_STATIC_MEMBER");
    case GRAMINA_AST_OP_PROPERTY:
        return mk_sv_c("OP_PROPERTY");
    case GRAMINA_AST_OP_MEMBER:
        return mk_sv_c("OP_MEMBER");
    case GRAMINA_AST_OP_CALL:
        return mk_sv_c("OP_CALL");
    case GRAMINA_AST_OP_SUBSCRIPT:
        return mk_sv_c("OP_SUBSCRIPT");
    case GRAMINA_AST_OP_SLICE:
        return mk_sv_c("OP_SLICE");

    case GRAMINA_AST_OP_EVAL:
        return mk_sv_c("OP_EVAL");
    case GRAMINA_AST_OP_RETHROW:
        return mk_sv_c("OP_RETHROW");

    case GRAMINA_AST_OP_CONCAT:
        return mk_sv_c("OP_CONCAT");

    case GRAMINA_AST_OP_UNARY_PLUS:
        return mk_sv_c("OP_UNARY_PLUS");
    case GRAMINA_AST_OP_UNARY_MINUS:
        return mk_sv_c("OP_UNARY_MINUS");
    case GRAMINA_AST_OP_LOGICAL_NOT:
        return mk_sv_c("OP_LOGICAL_NOT");
    case GRAMINA_AST_OP_DEREF:
        return mk_sv_c("OP_DEREF");
    case GRAMINA_AST_OP_ADDRESS_OF:
        return mk_sv_c("OP_ADDRESS_OF");
    case GRAMINA_AST_OP_ALTERNATE_NOT:
        return mk_sv_c("OP_ALTERNATE_NOT");

    case GRAMINA_AST_OP_MUL:
        return mk_sv_c("OP_MUL");
    case GRAMINA_AST_OP_DIV:
        return mk_sv_c("OP_DIV");
    case GRAMINA_AST_OP_REM:
        return mk_sv_c("OP_REM");

    case GRAMINA_AST_OP_ADD:
        return mk_sv_c("OP_ADD");
    case GRAMINA_AST_OP_SUB:
        return mk_sv_c("OP_SUB");

    case GRAMINA_AST_OP_LSHIFT:
        return mk_sv_c("OP_LSHIFT");
    case GRAMINA_AST_OP_RSHIFT:
        return mk_sv_c("OP_RSHIFT");

    case GRAMINA_AST_OP_INSERT:
        return mk_sv_c("OP_INSERT");
    case GRAMINA_AST_OP_EXTRACT:
        return mk_sv_c("OP_EXTRACT");

    case GRAMINA_AST_OP_LT:
        return mk_sv_c("OP_LT");
    case GRAMINA_AST_OP_LTE:
        return mk_sv_c("OP_LTE");
    case GRAMINA_AST_OP_GT:
        return mk_sv_c("OP_GT");
    case GRAMINA_AST_OP_GTE:
        return mk_sv_c("OP_GTE");

    case GRAMINA_AST_OP_EQUAL:
        return mk_sv_c("OP_EQUAL");
    case GRAMINA_AST_OP_INEQUAL:
        return mk_sv_c("OP_INEQUAL");

    case GRAMINA_AST_OP_LOGICAL_AND:
        return mk_sv_c("OP_LOGICAL_AND");
    case GRAMINA_AST_OP_LOGICAL_XOR:
        return mk_sv_c("OP_LOGICAL_XOR");
    case GRAMINA_AST_OP_LOGICAL_OR:
        return mk_sv_c("OP_LOGICAL_OR");

    case GRAMINA_AST_OP_ALTERNATE_AND:
        return mk_sv_c("OP_ALTERNATE_AND");
    case GRAMINA_AST_OP_ALTERNATE_XOR:
        return mk_sv_c("OP_ALTERNATE_XOR");
    case GRAMINA_AST_OP_ALTERNATE_OR:
        return mk_sv_c("OP_ALTERNATE_OR");

    case GRAMINA_AST_OP_FALLBACK:
        return mk_sv_c("OP_FALLBACK");

    case GRAMINA_AST_OP_ASSIGN:
        return mk_sv_c("OP_ASSIGN");
    case GRAMINA_AST_OP_ASSIGN_ADD:
        return mk_sv_c("OP_ASSIGN_ADD");
    case GRAMINA_AST_OP_ASSIGN_SUB:
        return mk_sv_c("OP_ASSIGN_SUB");
    case GRAMINA_AST_OP_ASSIGN_MUL:
        return mk_sv_c("OP_ASSIGN_MUL");
    case GRAMINA_AST_OP_ASSIGN_DIV:
        return mk_sv_c("OP_ASSIGN_DIV");
    case GRAMINA_AST_OP_ASSIGN_REM:
        return mk_sv_c("OP_ASSIGN_REM");
    case GRAMINA_AST_OP_ASSIGN_CAT:
        return mk_sv_c("OP_ASSIGN_CAT");

    default:
        return mk_sv_c("UNKNOWN");
    }
}

