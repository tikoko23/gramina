#define GRAMINA_NO_NAMESPACE

#include "cli/etc.h"
#include "cli/state.h"
#include "cli/tu.h"

#include "common/arg.h"
#include "common/init.h"
#include "common/log.h"

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

    if (S.sources.length > 1) {
        wlog_fmt("Multiple sources aren't supported. Only compiling '{cstr}'\n", S.sources.items[0]);
    }

    // TODO: multiple sources

    TranslationUnit T = {
        .file = S.sources.items[0],
    };

    Pipeline P = pipeline_default(&S);

    if (tu_pipe(&S, &P, &T)) {
        tu_free(&T);
        pipeline_free(&P);
        cli_state_free(&S);
    }

    tu_free(&T);
    pipeline_free(&P);
    cli_state_free(&S);

    return 0;
}
