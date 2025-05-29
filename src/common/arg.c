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
        if (!arg->shortened && sv_cmp_c(wanted, arg->argname) == 0) {
            return arg;
        }
    }

    return NULL;
}

static ArgumentInfo *find_flag(const Arguments *args, char wanted) {
    array_foreach_ref(_GraminaArgInfo, _, arg, args->named) {
        if (arg->shortened && arg->flag == wanted) {
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
                info->shortened
                    ? mk_sv_buf((uint8_t *)&info->flag, 1)
                    : mk_sv_c(info->argname)
            );

            return true;
        }

        return false;
    }

    char *potential_arg = argv[*i + 1];
    if (potential_arg[0] != '-') {
        info->param = potential_arg;
        ++(*i);
    } else if (info->param_needs == GRAMINA_PARAM_REQUIRED) {
        args->error = str_cfmt(
            "Argument '{svo}' requires a parameter",
            info->shortened
                ? mk_sv_buf((uint8_t *)&info->flag, 1)
                : mk_sv_c(info->argname)
        );

        return true;
    }

    return false;
}

bool gramina_parse_args(int argc, char **argv, Arguments *wanted) {
    for (size_t i = 0; i < argc; ++i) {
        char *this = argv[i];

        if (strcmp(this, "--") == 0) {
            populate_suffix_args(argc - i - 1, argv + i + 1, &wanted->suffix_args);
            break;
        }

        if (this[0] == '-' && this[1] == '-') {
            StringView long_arg = mk_sv_c(this + 2);

            ArgumentInfo *info = find_named(wanted, &long_arg);
            if (!info) {
                wanted->error = str_cfmt("Unknown argument '{sv}'\n", &long_arg);
                return true;
            }

            info->found = true;

            if (populate_arg(argc, argv, &i, info, wanted)) {
                return true;
            }

            continue;
        }

        if (this[0] == '-') {
            StringView flags = mk_sv_c(this + 1);

            if (flags.length == 0) {
                wanted->error = mk_str_c("Trailing '-'");
                return true;
            }

            sv_foreach(j, flag, flags) {
                ArgumentInfo *info = find_flag(wanted, flag);
                if (!info) {
                    wanted->error = str_cfmt("Unknown flag '{c}'\n", flag); 
                    return true;
                }

                info->found = true;

                if (j + 1 == flags.length) {
                    bool status = populate_arg(argc, argv, &i, info, wanted);
                    if (status) {
                        return true;
                    }
                }
            }

            continue;
        }

        array_append(_GraminaArgString, &wanted->positional, this);
    }

    return false;
}
