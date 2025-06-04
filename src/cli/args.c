#define GRAMINA_NO_NAMESPACE

#include <string.h>

#include "cli/etc.h"
#include "cli/help.h"
#include "cli/state.h"

#include "common/log.h"

enum {
    HELP_ARG,
    VERBOSE_ARG,
    OUT_FILE_ARG,
    LOG_LEVEL_ARG,
    AST_DUMP_ARG,
    IR_DUMP_ARG,
    STAGE_ARG,
    LINK_LIB_ARG,
    LINKER_PROG_ARG,
    KEEP_TEMPS_ARG,
};

static bool determine_log_level(Arguments *args) {
    ArgumentInfo *log_level_arg = &args->named.items[LOG_LEVEL_ARG];
    ArgumentInfo *verbose_arg = &args->named.items[VERBOSE_ARG];

    if (verbose_arg->found) {
        gramina_global_log_level = GRAMINA_LOG_LEVEL_VERBOSE;
    }

    if (log_level_arg->found) {
        if (false) {
        } else if (strcmp(log_level_arg->param, "all") == 0) {
            gramina_global_log_level = GRAMINA_LOG_LEVEL_VERBOSE;
        } else if (strcmp(log_level_arg->param, "info") == 0) {
            gramina_global_log_level = GRAMINA_LOG_LEVEL_INFO;
        } else if (strcmp(log_level_arg->param, "warn") == 0) {
            gramina_global_log_level = GRAMINA_LOG_LEVEL_WARN;
        } else if (strcmp(log_level_arg->param, "error") == 0) {
            gramina_global_log_level = GRAMINA_LOG_LEVEL_ERROR;
        } else if (strcmp(log_level_arg->param, "silent") == 0) {
            gramina_global_log_level = GRAMINA_LOG_LEVEL_NONE;
        } else if (strcmp(log_level_arg->param, "none") == 0) {
            gramina_global_log_level = GRAMINA_LOG_LEVEL_NONE + 1;
        } else {
            elog_fmt("Unknown log level '{cstr}'\n", log_level_arg->param);
            return true;
        }

        if (verbose_arg->found) {
            wlog_fmt("'--log-level' will override '--verbose' and '-v'\n");
        }
    }

    return false;
}

static bool determine_max_stage(CliState *S, Arguments *args) {
    ArgumentInfo *stage_arg = &args->named.items[STAGE_ARG];

    if (!stage_arg->found) {
        S->max_stage = COMPILATION_STAGE_BIN;
        return false;
    }

    if (false) {
    } else if (strcmp(stage_arg->param, "a") == 0
            || strcmp(stage_arg->param, "asm") == 0
            || strcmp(stage_arg->param, "assembly") == 0) {
        S->max_stage = COMPILATION_STAGE_ASM;
    } else if (strcmp(stage_arg->param, "o") == 0
            || strcmp(stage_arg->param, "obj") == 0
            || strcmp(stage_arg->param, "object") == 0) {
        S->max_stage = COMPILATION_STAGE_OBJ;
    } else if (strcmp(stage_arg->param, "b") == 0
            || strcmp(stage_arg->param, "bin") == 0
            || strcmp(stage_arg->param, "binary") == 0) {
        S->max_stage = COMPILATION_STAGE_BIN;
    } else {
        elog_fmt("Unknown stage '{cstr}'\n", stage_arg->param);
    }

    return false;
}

static bool populate_fields(CliState *S, Arguments *args) {
    ArgumentInfo *help_arg = &args->named.items[HELP_ARG];

    if (help_arg->found) {
        if (!help_arg->param) {
            cli_show_general_help();
        } else {
            cli_show_specific_help(help_arg->param);
        }

        S->wants_help = true;

        return false;
    }

    if (args->positional.length == 0) {
        elog_fmt("Missing file argument\n");
        return true;
    }

    ArgumentInfo *out_file_arg = &args->named.items[OUT_FILE_ARG];
    ArgumentInfo *ast_dump_arg = &args->named.items[AST_DUMP_ARG];
    ArgumentInfo *ir_dump_arg = &args->named.items[IR_DUMP_ARG];
    ArgumentInfo *linker_prog_arg = &args->named.items[LINKER_PROG_ARG];
    ArgumentInfo *keep_temps_arg = &args->named.items[KEEP_TEMPS_ARG];

    S->sources = args->positional;
    args->positional = mk_array(_GraminaArgString); // Move the array

    S->out_file = out_file_arg->found
                ? out_file_arg->param
                : "a.out";

    S->ast_dump_file = ast_dump_arg->found
                     ? ast_dump_arg->param
                     : NULL;

    S->ir_dump_file = ir_dump_arg->found
                    ? ir_dump_arg->param
                    : NULL;

    S->linker_prog = linker_prog_arg->found
                   ? linker_prog_arg->param
                   : "ld"; // Only works with GNU toolchain

    S->keep_temps = keep_temps_arg->found;

    if (determine_log_level(args)) {
        return true;
    }

    if (determine_max_stage(S, args)) {
        return true;
    }

    return false;
}

bool cli_handle_args(CliState *S, int argc, char **argv) {
    ArgumentInfo argument_info[] = {
        [HELP_ARG] = {
            .name = "help",
            .type = GRAMINA_ARG_LONG,
            .param_needs = GRAMINA_PARAM_OPTIONAL,
            .override_behavior = GRAMINA_OVERRIDE_FORBID,
        },
        [VERBOSE_ARG] = {
            .name = "verbose",
            .flag = 'v',
            .type = GRAMINA_ARG_LONG | GRAMINA_ARG_FLAG,
            .param_needs = GRAMINA_PARAM_NONE,
            .override_behavior = GRAMINA_OVERRIDE_OK,
        },
        [OUT_FILE_ARG] = {
            .flag = 'o',
            .type = GRAMINA_ARG_FLAG,
            .param_needs = GRAMINA_PARAM_REQUIRED,
            .override_behavior = GRAMINA_OVERRIDE_FORBID,
        },
        [STAGE_ARG] = {
            .flag = 's',
            .name = "stage",
            .type = GRAMINA_ARG_FLAG | GRAMINA_ARG_LONG,
            .param_needs = GRAMINA_PARAM_REQUIRED,
            .override_behavior = GRAMINA_OVERRIDE_FORBID,
        },
        [LOG_LEVEL_ARG] = {
            .name = "log-level",
            .type = GRAMINA_ARG_LONG,
            .param_needs = GRAMINA_PARAM_REQUIRED,
            .override_behavior = GRAMINA_OVERRIDE_FORBID,
        },
        [AST_DUMP_ARG] = {
            .name = "ast-dump",
            .type = GRAMINA_ARG_LONG,
            .param_needs = GRAMINA_PARAM_REQUIRED,
            .override_behavior = GRAMINA_OVERRIDE_FORBID,
        },
        [IR_DUMP_ARG] = {
            .name = "ir-dump",
            .type = GRAMINA_ARG_LONG,
            .param_needs = GRAMINA_PARAM_REQUIRED,
            .override_behavior = GRAMINA_OVERRIDE_FORBID,
        },
        [LINK_LIB_ARG] = {
            .flag = 'l',
            .type = GRAMINA_ARG_FLAG,
            .param_needs = GRAMINA_PARAM_MULTI,
            .override_behavior = GRAMINA_OVERRIDE_OK,
            .multi_params = &S->link_libs,
        },
        [LINKER_PROG_ARG] = {
            .name = "linker",
            .type = GRAMINA_ARG_LONG,
            .param_needs = GRAMINA_PARAM_REQUIRED,
            .override_behavior = GRAMINA_OVERRIDE_WARN,
        },
        [KEEP_TEMPS_ARG] = {
            .name = "keep-temps",
            .type = GRAMINA_ARG_LONG,
            .param_needs = GRAMINA_PARAM_NONE,
            .override_behavior = GRAMINA_OVERRIDE_OK,
        },
    };

    Arguments args = {
        .named = mk_array_c(_GraminaArgInfo, argument_info, (sizeof argument_info) / (sizeof argument_info[0])),
    };

    if (args_parse(&args, argc, argv)) {
        elog_fmt("Arguments: {s}\n", &args.error);
        args_free(&args);
        return true;
    }

    if (populate_fields(S, &args)) {
        args_free(&args);
        return true;
    }

    args_free(&args);
    return false;
}
