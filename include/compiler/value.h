#ifndef __GRAMINA_COMPILER_DECL_H
#define __GRAMINA_COMPILER_DECL_H

#include <stdint.h>

#include <llvm-c/Types.h>

#include "compiler/type.h"

enum gramina_value_class {
    GRAMINA_CLASS_INVALID,
    GRAMINA_CLASS_ALLOCA,
    GRAMINA_CLASS_LVALUE,
    GRAMINA_CLASS_RVALUE,
    GRAMINA_CLASS_CONSTEXPR,
};

struct gramina_value {
    union {
        LLVMValueRef lvalue_ptr;
    };

    enum gramina_value_class class;
    LLVMValueRef llvm;
    struct gramina_type type;
};

union gramina_primitive_initialiser {
    bool boolean;

    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;

    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;

    float f32;
    double f64;
};

bool gramina_value_is_valid(const struct gramina_value *this);
void gramina_value_free(struct gramina_value *this);
struct gramina_value gramina_value_dup(const struct gramina_value *this);
struct gramina_value gramina_invalid_value();

struct gramina_value gramina_mk_primitive_value(enum gramina_primitive primitive, union gramina_primitive_initialiser val);

#endif
#include "gen/compiler/value.h"
