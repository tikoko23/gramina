#include <setjmp.h>
#include <string.h>

#include "tester.h"
#include "tests.h"

#include "common/def.h"
#include "common/log.h"
#include "common/init.h"
#include "compiler/compiler.h"

#ifdef GRAMINA_UNIX_BUILD
#  include <unistd.h>
#endif

static int run_tests(const Test *tests, size_t n) {
    size_t n_passed = 0;
    size_t n_failed = 0;

    for (size_t i = 0; i < n; ++i) {
        const Test *const T = tests + i;

        switch (setjmp(tester_ret)) {
        case 0:
            T->tester();
            wlog_fmt("Test '{cstr}' did not terminate properly, missing `test_ok` or `test_fail`?\n", T->name);
            break;
        case TEST_FAIL:
            ++n_failed;
            log_fmt(GRAMINA_LOG_LEVEL_NONE, "Test '{cstr}' failed!\n", T->name);
            break;
        case TEST_SUCCESS:
            ++n_passed;
            ilog_fmt("\x1b[32mTest '{cstr}' passed!\x1b[0m\n", T->name);
            break;
        }
    }

    LogLevel level = n_failed > 0
                   ? GRAMINA_LOG_LEVEL_ERROR
                   : GRAMINA_LOG_LEVEL_INFO;

    log_fmt(level, "{sz} passed, {sz} failed\n", n_passed, n_failed);

    // This case probably won't ever happen but why not?
    return n_failed == 0 || n_failed % 256 != 0
         ? n_failed
         : 1;
}

static void switch_to_test_dir() {
#ifdef GRAMINA_UNIX_BUILD
    const char *self = __FILE__;
    
    char *loc = strrchr(self, '/');

    size_t filename_start = loc - self;
    char buf[filename_start + 1];
    memcpy(buf, self, filename_start);
    buf[filename_start] = '\0';

    printf("Test directory: %s\n", buf);

    chdir(buf);
#endif
}

int main(int argc, char **argv) {
    if (argc == 0) {
        elog_fmt("argc is 0\n");
        return 1;
    }

    switch_to_test_dir();

    init();
    compiler_init();

    gramina_global_log_level = GRAMINA_LOG_LEVEL_VERBOSE;

    if (!getenv("GRAMINA")) {
        wlog_fmt("If the build folder is not './build', please provide the 'GRAMINA' environment variable relative to the test directory for compiler tests\n"
                 "   This message does not necessarily imply a faulty configuration\n");
    }

    Test tests[] = {
        MAKE_TEST(Hello),
        MAKE_TEST(Exit),
        MAKE_TEST(TypeConstructor),
        MAKE_TEST(SStream),
        MAKE_TEST(Constness),
        MAKE_TEST(ArrayBadLen),
        MAKE_TEST(ArrayOk),
        MAKE_TEST(Pipe),
        MAKE_TEST(SliceRef),
    };

    size_t n_tests = (sizeof tests) / (sizeof tests[0]);
    ilog_fmt("Found {sz} tests\n", n_tests);

    return run_tests(tests, n_tests);
}
