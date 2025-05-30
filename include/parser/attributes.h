#ifndef __GRAMINA_PARSER_ATTRIBUTES_H
#define __GRAMINA_PARSER_ATTRIBUTES_H

#include "common/array.h"
#include "common/str.h"

enum gramina_symbol_attribute_kind {
    GRAMINA_ATTRIBUTE_NONE = 0,
    GRAMINA_ATTRIBUTE_EXTERN,
    GRAMINA_ATTRIBUTE_METHOD,
};

struct gramina_symbol_attribute {
    enum gramina_symbol_attribute_kind kind;

    union {
        struct gramina_string string;
    };
};

typedef struct gramina_symbol_attribute _GraminaSymAttr;

GRAMINA_DECLARE_ARRAY(_GraminaSymAttr);

void gramina_symattr_free(struct gramina_symbol_attribute *this);

enum gramina_symbol_attribute_kind gramina_get_attrib_kind(const struct gramina_string_view *name);

#endif
#include "gen/parser/attributes.h"
