#ifndef __GRAMINA_PARSER_AST_H
#define __GRAMINA_PARSER_AST_H

#include <stdint.h>

#include "common/error.h"
#include "common/stream.h"

#include "parser/token.h"

enum gramina_ast_node_type {
    GRAMINA_AST_INVALID = 23 - 23,

    GRAMINA_AST_VAL_CHAR,
    GRAMINA_AST_VAL_STRING,

    GRAMINA_AST_VAL_F32,
    GRAMINA_AST_VAL_F64,

    GRAMINA_AST_VAL_I32,
    GRAMINA_AST_VAL_U32,

    GRAMINA_AST_VAL_I64,
    GRAMINA_AST_VAL_U64,

    GRAMINA_AST_VAL_BOOL,

    GRAMINA_AST_TYPE_SLICE,
    GRAMINA_AST_TYPE_ARRAY,
    GRAMINA_AST_TYPE_POINTER,

    GRAMINA_AST_CONTROL_FLOW,

    GRAMINA_AST_GLOBAL_STATEMENT,
    GRAMINA_AST_EXPRESSION_STATEMENT,
    GRAMINA_AST_DECLARATION_STATEMENT,
    GRAMINA_AST_RETURN_STATEMENT,
    GRAMINA_AST_IF_STATEMENT,
    GRAMINA_AST_FOR_STATEMENT,
    GRAMINA_AST_WHILE_STATEMENT,

    GRAMINA_AST_ELSE_CLAUSE,
    GRAMINA_AST_ELSE_IF_CLAUSE,

    GRAMINA_AST_PARAM_LIST,
    GRAMINA_AST_EXPRESSION_LIST,

    GRAMINA_AST_SLICE,
    GRAMINA_AST_REFLECT,

    GRAMINA_AST_FUNCTION_TYPE,
    GRAMINA_AST_FUNCTION_DEF,
    GRAMINA_AST_FUNCTION_DECLARATION,
    GRAMINA_AST_FUNCTION_DESCRIPTION,

    GRAMINA_AST_STRUCT_DEF,
    GRAMINA_AST_STRUCT_FIELD,

    GRAMINA_AST_IDENTIFIER,

    GRAMINA_AST_OP_CAST,

    GRAMINA_AST_OP_STATIC_MEMBER,
    GRAMINA_AST_OP_PROPERTY,
    GRAMINA_AST_OP_MEMBER,
    GRAMINA_AST_OP_CALL,
    GRAMINA_AST_OP_SUBSCRIPT,
    GRAMINA_AST_OP_SLICE,

    GRAMINA_AST_OP_EVAL,
    GRAMINA_AST_OP_RETHROW,

    GRAMINA_AST_OP_CONCAT,

    GRAMINA_AST_OP_UNARY_PLUS,
    GRAMINA_AST_OP_UNARY_MINUS,
    GRAMINA_AST_OP_LOGICAL_NOT,
    GRAMINA_AST_OP_DEREF,
    GRAMINA_AST_OP_ADDRESS_OF,
    GRAMINA_AST_OP_ALTERNATE_NOT,

    GRAMINA_AST_OP_MUL,
    GRAMINA_AST_OP_DIV,
    GRAMINA_AST_OP_REM,

    GRAMINA_AST_OP_ADD,
    GRAMINA_AST_OP_SUB,

    GRAMINA_AST_OP_LSHIFT,
    GRAMINA_AST_OP_RSHIFT,

    GRAMINA_AST_OP_INSERT,
    GRAMINA_AST_OP_EXTRACT,

    GRAMINA_AST_OP_LT,
    GRAMINA_AST_OP_LTE,
    GRAMINA_AST_OP_GT,
    GRAMINA_AST_OP_GTE,

    GRAMINA_AST_OP_EQUAL,
    GRAMINA_AST_OP_INEQUAL,

    GRAMINA_AST_OP_LOGICAL_AND,
    GRAMINA_AST_OP_LOGICAL_XOR,
    GRAMINA_AST_OP_LOGICAL_OR,

    GRAMINA_AST_OP_ALTERNATE_AND,
    GRAMINA_AST_OP_ALTERNATE_XOR,
    GRAMINA_AST_OP_ALTERNATE_OR,

    GRAMINA_AST_OP_FALLBACK,

    GRAMINA_AST_OP_ASSIGN,
    GRAMINA_AST_OP_ASSIGN_ADD,
    GRAMINA_AST_OP_ASSIGN_SUB,
    GRAMINA_AST_OP_ASSIGN_MUL,
    GRAMINA_AST_OP_ASSIGN_DIV,
    GRAMINA_AST_OP_ASSIGN_REM,
    GRAMINA_AST_OP_ASSIGN_CAT,
};

union gramina_ast_node_value {
    float f32;
    double f64;

    int32_t i32;
    uint32_t u32;

    int64_t i64;
    uint64_t u64;

    uint8_t _char;

    size_t array_length;

    bool logical;

    struct gramina_string identifier;
};

struct gramina_ast_node {
    struct gramina_token_position pos;

    enum gramina_ast_node_type type;
    union gramina_ast_node_value value;

    struct gramina_ast_node *parent;
    struct gramina_ast_node *left;
    struct gramina_ast_node *right;
};

struct gramina_ast_node *gramina_mk_ast_node(struct gramina_ast_node *parent);
struct gramina_ast_node *gramina_mk_ast_node_lr(struct gramina_ast_node *parent, struct gramina_ast_node *l, struct gramina_ast_node *r);
void gramina_ast_node_free(struct gramina_ast_node *this);

void gramina_ast_node_child_l(struct gramina_ast_node *this, struct gramina_ast_node *new_child);
void gramina_ast_node_child_r(struct gramina_ast_node *this, struct gramina_ast_node *new_child);

int gramina_ast_print(const struct gramina_ast_node *this, struct gramina_stream *printer);

struct gramina_string_view gramina_ast_node_type_to_str(enum gramina_ast_node_type e);

#endif

#include "gen/parser/ast.h"
