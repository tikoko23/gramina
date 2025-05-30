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
