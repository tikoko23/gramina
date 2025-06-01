#define GRAMINA_NO_NAMESPACE

#include "cli/etc.h"
#include "cli/state.h"

#include "common/arg.h"
#include "common/init.h"
#include "common/log.h"

int _main(int argc, char **argv) {
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

    cli_state_free(&S);

    return 0;
}
