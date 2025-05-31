#define GRAMINA_NO_NAMESPACE

#include <string.h>

#include "common/arg.h"

GRAMINA_IMPLEMENT_ARRAY(_GraminaArgInfo)
GRAMINA_IMPLEMENT_ARRAY(_GraminaArgString)

static void populate_suffix_args(int argc, char **argv, Array(_GraminaArgString) *wanted) {
    if (argc <= 0) {
        return;
    }

    for (size_t i = 0; i < argc; ++i) {
        array_append(_GraminaArgString, wanted, argv[i]);
    }
}

static ArgumentInfo *find_named(const Arguments *args, const StringView *wanted) {
    array_foreach_ref(_GraminaArgInfo, _, arg, args->named) {
        if ((arg->type & GRAMINA_ARG_LONG) && sv_cmp_c(wanted, arg->name) == 0) {
            return arg;
        }
    }

    return NULL;
}

static ArgumentInfo *find_flag(const Arguments *args, char wanted) {
    array_foreach_ref(_GraminaArgInfo, _, arg, args->named) {
        if ((arg->type & GRAMINA_ARG_FLAG) && arg->flag == wanted) {
            return arg;
        }
    }

    return NULL;
}

static bool populate_arg(int argc, char **argv, size_t *i, ArgumentInfo *info, Arguments *args) {
    if (info->param_needs == GRAMINA_PARAM_NONE || *i + 1 >= argc) {
        if (info->param_needs == GRAMINA_PARAM_REQUIRED) {
            args->error = str_cfmt(
                "Argument '{svo}' requires a parameter",
                info->type & GRAMINA_ARG_LONG
                    ? mk_sv_c(info->name)
                    : mk_sv_buf((uint8_t *)&info->flag, 1)
            );

            return true;
        }

        return false;
    }

    char *potential_arg = argv[*i + 1];
    if (potential_arg[0] != '-') {
        if (info->param_needs != GRAMINA_PARAM_MULTI) {
            info->param = potential_arg;
        } else {
            array_append(_GraminaArgString, info->multi_params, potential_arg);
        }

        ++(*i);
    } else if (info->param_needs == GRAMINA_PARAM_REQUIRED
            || info->param_needs == GRAMINA_PARAM_MULTI) {
        args->error = str_cfmt(
            "Argument '{svo}' requires a parameter",
            info->type & GRAMINA_ARG_LONG
                ? mk_sv_c(info->name)
                : mk_sv_buf((uint8_t *)&info->flag, 1)
        );

        return true;
    }

    return false;
}

bool gramina_args_parse(Arguments *this, int argc, char **argv) {
    // I don't know if this can happen but it's here anyway
    if (argc <= 0) {
        return true;
    }

    this->exec = argv[0];

    for (size_t i = 1; i < argc; ++i) {
        char *current = argv[i];

        if (strcmp(current, "--") == 0) {
            populate_suffix_args(argc - i - 1, argv + i + 1, &this->suffix_args);
            break;
        }

        if (current[0] == '-' && current[1] == '-') {
            StringView long_arg = mk_sv_c(current + 2);

            ArgumentInfo *info = find_named(this, &long_arg);
            if (!info) {
                this->error = str_cfmt("Unknown argument '{sv}'", &long_arg);
                return true;
            }

            info->found = true;

            if (populate_arg(argc, argv, &i, info, this)) {
                return true;
            }

            continue;
        }

        if (current[0] == '-') {
            StringView flags = mk_sv_c(current + 1);

            if (flags.length == 0) {
                this->error = mk_str_c("Trailing '-'");
                return true;
            }

            sv_foreach(j, flag, flags) {
                ArgumentInfo *info = find_flag(this, flag);
                if (!info) {
                    this->error = str_cfmt("Unknown flag '{c}'", flag);
                    return true;
                }

                info->found = true;

                if (j + 1 == flags.length) {
                    bool status = populate_arg(argc, argv, &i, info, this);
                    if (status) {
                        return true;
                    }
                }
            }

            continue;
        }

        array_append(_GraminaArgString, &this->positional, current);
    }

    return false;
}

void gramina_args_free(Arguments *this) {
    array_free(_GraminaArgInfo, &this->named);
    array_free(_GraminaArgString, &this->positional);
    array_free(_GraminaArgString, &this->suffix_args);
    str_free(&this->error);
}
