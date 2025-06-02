#define GRAMINA_NO_NAMESPACE

#include "cli/etc.h"
#include "cli/state.h"
#include "cli/tu.h"

#include "common/arg.h"
#include "common/init.h"
#include "common/log.h"

#ifdef GRAMINA_UNIX_BUILD
#  undef GRAMINA_UNIX_BUILD
#  define GRAMINA_UNIX_BUILD 1
#else
#  define GRAMINA_UNIX_BUILD 0
#endif

int main(int argc, char **argv) {
    init();
    atexit(cleanup);

    CliState S = {
        .link_libs = mk_array(_GraminaArgString),
        .sources = mk_array(_GraminaArgString),
    };

    if (cli_handle_args(&S, argc, argv)) {
        cli_state_free(&S);
        return 1;
    }

    if (!GRAMINA_UNIX_BUILD && S.max_stage == COMPILATION_STAGE_BIN) {
        log_fmt(GRAMINA_LOG_LEVEL_NONE, "Emitting native binaries isn't supported on non-POSIX systems\n"
                                        "!! Manually link object files instead\n"
                                        "!! To generate object files, use `gramina --stage object`\n");

        cli_state_free(&S);
        return 1;
    }

    Pipeline P = pipeline_default(&S);

    TranslationUnit tus[S.sources.length];

    array_foreach(_GraminaArgString, i, source, S.sources) {
        TranslationUnit T = {
            .file = source,
        };

        if (tu_pipe(&S, &P, &T)) {
            tu_free(&T);
            pipeline_free(&P);
            cli_state_free(&S);

            return 1;
        }

        tus[i] = T;
    }

    const size_t length = (sizeof tus) / (sizeof tus[0]);

    if (!(S.machine = cli_get_machine())) {
        pipeline_free(&P);
        cli_state_free(&S);

        for (size_t i = 0; i < length; ++i) {
            tu_free(tus + i);
        }

        return 1;
    }

    bool err = false;
    const char *emit_type;
    if (S.max_stage == COMPILATION_STAGE_OBJ
     || S.max_stage == COMPILATION_STAGE_ASM) {
        emit_type = S.max_stage == COMPILATION_STAGE_OBJ
                  ? "object"
                  : "assembly";

        err = tu_emit_objects(
            &S, tus, length,
            S.max_stage == COMPILATION_STAGE_OBJ
                ? OBJECT_FILE
                : ASM_FILE
        );
    } else {
        emit_type = "binary";
        err = tu_emit_binary(&S, tus, length);
    }

    int status = 0;
    if (err) {
        elog_fmt("Failed to emit {cstr} file\n", emit_type);
        status = 1; 
    }

    for (size_t i = 0; i < length; ++i) {
        tu_free(tus + i);
    }

    pipeline_free(&P);
    cli_state_free(&S);

    return status;
}
