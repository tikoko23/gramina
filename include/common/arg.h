#ifndef __GRAMINA_COMMON_ARG_H
#define __GRAMINA_COMMON_ARG_H

#include <stdbool.h>

#include "common/array.h"
#include "common/hashmap.h"
#include "common/str.h"

struct gramina_argument_info {
    bool shortened;
    enum {
        GRAMINA_PARAM_NONE,
        GRAMINA_PARAM_OPTIONAL,
        GRAMINA_PARAM_REQUIRED,
    } param_needs;
    union {
        struct {
            char flag;
        };
        struct {
            const char *argname;
        };
    };

    bool found;
    const char *param;
};

typedef struct gramina_argument_info _GraminaArgInfo;
typedef const char *_GraminaArgString;

GRAMINA_DECLARE_ARRAY(_GraminaArgInfo);
GRAMINA_DECLARE_ARRAY(_GraminaArgString);

struct gramina_arguments {
    struct gramina_array(_GraminaArgString) positional;
    struct gramina_array(_GraminaArgInfo) named;
    struct gramina_array(_GraminaArgString) suffix_args;
    struct gramina_string error;
};

// Returns whether an error occured
bool gramina_parse_args(int argc, char **argv, struct gramina_arguments *wanted); 

#endif
#include "gen/common/arg.h"
