#ifndef __GRAMINA_COMMON_ARG_H
#define __GRAMINA_COMMON_ARG_H

#include <stdbool.h>

#include "common/array.h"
#include "common/def.h"
#include "common/hashmap.h"
#include "common/str.h"

typedef struct gramina_argument_info _GraminaArgInfo;
typedef const char *_GraminaArgString;

GRAMINA_DECLARE_ARRAY(_GraminaArgInfo);
GRAMINA_DECLARE_ARRAY(_GraminaArgString);

enum {
    GRAMINA_ARG_LONG = GRAMINA_N_TH(0),
    GRAMINA_ARG_FLAG = GRAMINA_N_TH(1),
};

struct gramina_argument_info {
    uint32_t type;

    enum {
        GRAMINA_PARAM_NONE,
        GRAMINA_PARAM_OPTIONAL,
        GRAMINA_PARAM_REQUIRED,
        GRAMINA_PARAM_MULTI,
    } param_needs;

    enum {
        GRAMINA_OVERRIDE_OK,
        GRAMINA_OVERRIDE_WARN,
        GRAMINA_OVERRIDE_FORBID,
    } override_behavior;

    char flag;
    const char *name;

    bool found;
    const char *param;

    struct gramina_array(_GraminaArgString) *multi_params;
};

struct gramina_arguments {
    const char *exec;
    struct gramina_array(_GraminaArgString) positional;
    struct gramina_array(_GraminaArgInfo) named;
    struct gramina_array(_GraminaArgString) suffix_args;
    struct gramina_string error;
};

// Returns whether an error occured
bool gramina_args_parse(struct gramina_arguments *this, int argc, char **argv);
void gramina_args_free(struct gramina_arguments *this);

#endif
#include "gen/common/arg.h"
