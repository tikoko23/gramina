#define GRAMINA_NO_NAMESPACE

#include <string.h>

#include "cli/etc.h"
#include "cli/state.h"

#include "common/log.h"
#include "common/subprocess.h"

static const char *get_linker(CliState *S) {
    char *from_env = getenv("GRAMINA_LINKER");
    if (from_env) {
        return from_env;
    }

    return S->linker_prog;
}

bool cli_link_objects(CliState *S, const StringView *files, size_t n_files) {
    const char *linker = get_linker(S);

    Subprocess sp = mk_sbp();

    sbp_arg_cstr(&sp, linker);

    for (size_t i = 0; i < n_files; ++i) {
        sbp_arg_sv(&sp, files + i);
    }

    array_foreach(_GraminaArgString, _, lib, S->link_libs) {
        sbp_arg_cstr(&sp, "-l");
        sbp_arg_cstr(&sp, lib);
    }

    sbp_arg_cstr(&sp, "-o");
    sbp_arg_cstr(&sp, S->out_file);

    if (gramina_global_log_level <= GRAMINA_LOG_LEVEL_VERBOSE) {
        String cmd = mk_str();

        array_foreach_ref(_GraminaSbpArg, _, arg, sp.argv) {
            str_cat(&cmd, arg);
            str_append(&cmd, ' ');
        }

        str_pop(&cmd);

        vlog_fmt("Running '{so}'\n", cmd);
    }

    int status = sbp_run_sync(&sp);
    if (status) {
        elog_fmt("Child process: {cstr}\n", strerror(status));
        sbp_free(&sp);
        return true;
    }

    if (sp.exit_code) {
        elog_fmt("Child process exited with code {i32}\n", (int32_t)sp.exit_code);
        sbp_free(&sp);
        return true;
    }

    sbp_free(&sp);
    return false;
}
