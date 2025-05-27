#ifndef __GRAMINA_COMPILER_TYPEDECL_H
#define __GRAMINA_COMPILER_TYPEDECL_H

#include <llvm-c/Types.h>

#include <stdbool.h>
#include <stdint.h>

#include "parser/ast.h"
#include "compiler/scope.h"

enum gramina_primitive {
    GRAMINA_PRIMITIVE_BOOL,
    GRAMINA_PRIMITIVE_BYTE,
    GRAMINA_PRIMITIVE_UBYTE,
    GRAMINA_PRIMITIVE_SHORT,
    GRAMINA_PRIMITIVE_USHORT,
    GRAMINA_PRIMITIVE_INT,
    GRAMINA_PRIMITIVE_UINT,
    GRAMINA_PRIMITIVE_LONG,
    GRAMINA_PRIMITIVE_ULONG,
    GRAMINA_PRIMITIVE_FLOAT,
    GRAMINA_PRIMITIVE_DOUBLE,
};

enum gramina_type_kind {
    GRAMINA_TYPE_INVALID,
    GRAMINA_TYPE_VOID,
    GRAMINA_TYPE_PRIMITIVE,
    GRAMINA_TYPE_ARRAY,
    GRAMINA_TYPE_SLICE,
    GRAMINA_TYPE_POINTER,
    GRAMINA_TYPE_FUNCTION,
    GRAMINA_TYPE_STRUCT,
    GRAMINA_TYPE_GENERIC,

    GRAMINA_TYPE_GENERIC_PARAM,
};

struct gramina_type;
typedef struct gramina_type _GraminaType;

GRAMINA_DECLARE_ARRAY(_GraminaType);

struct gramina_type_requirements {

};

struct gramina_type {
    LLVMTypeRef llvm;
    enum gramina_type_kind kind;
    // bool lvalue;
    union {
        /* PRIMITIVE */ enum gramina_primitive primitive;
        /* FUNCTION */ struct {
            struct gramina_type *return_type;
            struct gramina_array(_GraminaType) param_types;
        };
        /* POINTER */ struct gramina_type *pointer_type;
        /* SLICE */ struct gramina_type *slice_type;
        /* STRUCT */ struct {
            struct gramina_hashmap fields;
            struct gramina_string struct_name;
        };
        /* GENERIC */ struct {
            struct gramina_array(_GraminaType) generic_params;
        };
        /* GENERIC_PARAM */ struct {
            struct gramina_type_requirements requirements;
            struct gramina_string generic_name;
        };
    };
};

struct gramina_coercion_result {
    struct gramina_type greater_type;
    bool is_right_promoted;
};

struct gramina_struct_field {
    struct gramina_type type;
    size_t index;
};

#endif
#include "gen/compiler/typedecl.h"
