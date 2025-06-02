#include "tester.h"

void TEST_Exit() {
    Subprocess sp = mk_sbp();
    sbp_arg_cstr(&sp, get_compiler());
    sbp_arg_cstr(&sp, "gramina/exit.lawn");
    set_compilation_output(&sp, "local/test_exit");
    add_libc(&sp);

    run_command(&sp);

    sbp_free(&sp);
    test_ok();
}
