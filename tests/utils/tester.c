#include <stdlib.h>

#include "tester.h"

#include "common/log.h"

jmp_buf tester_ret;

void test_ok() {
    longjmp(tester_ret, TEST_SUCCESS);
}

void test_fail() {
    longjmp(tester_ret, TEST_FAIL);
}

void test_fail_cmsg(const char *msg) {
    elog_fmt("Test failed: {cstr}\n", msg);
    test_fail();
}

void test_fail_msg(String msg) {
    elog_fmt("Test failed: {so}\n", msg);
    test_fail();
}

void run_command(Subprocess *sp) {
    if (sbp_run_sync(sp)) {
        test_fail_cmsg("Failed to summon child process");
    }

    if (sp->exit_code) {
        test_fail_msg(str_cfmt("Child process exited with status {i32}", (int32_t)sp->exit_code));
    }
}

void set_compilation_output(Subprocess *sp, const char *out) {
    sbp_arg_cstr(sp, "-o");
    sbp_arg_cstr(sp, out);
}

void add_libc(Subprocess *sp) {
    sbp_arg_cstr(sp, "-l");
    sbp_arg_cstr(sp, "c");
}

const char *get_compiler() {
    char *ret;
    if ((ret = getenv("GRAMINA"))) {
        return ret;
    }

    return "../build/gramina";
}
