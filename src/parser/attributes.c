#define GRAMINA_NO_NAMESPACE

#include "parser/attributes.h"

GRAMINA_IMPLEMENT_ARRAY(_GraminaSymAttr);

// GRAMINA_ATTRIBUTE_METHOD currently leaks memory.
// This will be fixed when method syntax is finalised
void gramina_symattr_free(SymbolAttribute *this) {
    switch (this->kind) {
    case GRAMINA_ATTRIBUTE_EXTERN:
        str_free(&this->string);
        break;
    default:
        break;
    }

    this->kind = GRAMINA_ATTRIBUTE_NONE;
}

SymbolAttributeKind gramina_get_attrib_kind(const StringView *name) {
    if (false) {
    } else if (sv_cmp_c(name, "extern") == 0) {
        return GRAMINA_ATTRIBUTE_EXTERN;
    } else if (sv_cmp_c(name, "method") == 0) {
        return GRAMINA_ATTRIBUTE_METHOD;
    }

    return GRAMINA_ATTRIBUTE_NONE;
}
