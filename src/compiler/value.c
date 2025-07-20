#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#define GRAMINA_NO_NAMESPACE

#include "compiler/value.h"

bool value_is_valid(const Value *this) {
    return this->type.kind != GRAMINA_TYPE_INVALID
        && this->llvm != NULL
        && this->class != GRAMINA_CLASS_INVALID;
}

void value_free(Value *this) {
    type_free(&this->type);
}

Value value_dup(const Value *this) {
    Value ret = *this;
    ret.type = type_dup(&this->type);

    return ret;
}

Value invalid_value() {
    return (Value) {
        .type = {
            .kind = GRAMINA_TYPE_INVALID,
        },
        .llvm = NULL,
        .class = GRAMINA_CLASS_INVALID,
    };
}

Value mk_primitive_value(Primitive primitive, PrimitiveInitialiser val) {
    LLVMValueRef const_val;
    switch (primitive) {
    case GRAMINA_PRIMITIVE_BOOL:
        const_val = LLVMConstInt(LLVMInt1Type(), val.boolean, false);
        break;
    case GRAMINA_PRIMITIVE_BYTE:
        const_val = LLVMConstInt(LLVMInt8Type(), val.i8, true);
        break;
    case GRAMINA_PRIMITIVE_UBYTE:
        const_val = LLVMConstInt(LLVMInt8Type(), val.u8, false);
        break;
    case GRAMINA_PRIMITIVE_SHORT:
        const_val = LLVMConstInt(LLVMInt16Type(), val.i16, true);
        break;
    case GRAMINA_PRIMITIVE_USHORT:
        const_val = LLVMConstInt(LLVMInt16Type(), val.u16, false);
        break;
    case GRAMINA_PRIMITIVE_INT:
        const_val = LLVMConstInt(LLVMInt32Type(), val.i32, true);
        break;
    case GRAMINA_PRIMITIVE_UINT:
        const_val = LLVMConstInt(LLVMInt32Type(), val.u32, false);
        break;
    case GRAMINA_PRIMITIVE_LONG:
        const_val = LLVMConstInt(LLVMInt64Type(), val.i64, true);
        break;
    case GRAMINA_PRIMITIVE_ULONG:
        const_val = LLVMConstInt(LLVMInt64Type(), val.u64, false);
        break;
    case GRAMINA_PRIMITIVE_FLOAT:
        const_val = LLVMConstReal(LLVMFloatType(), val.f32);
        break;
    case GRAMINA_PRIMITIVE_DOUBLE:
        const_val = LLVMConstReal(LLVMDoubleType(), val.f64);
        break;
    }

    return (Value) {
        .llvm = const_val,
        .type = type_from_primitive(primitive),
        .class = GRAMINA_CLASS_CONSTEXPR,
    };
}
