#include "tester.h"

TEST(ArrayBadLen) {
    Stream len_f = mk_stream_open_c("gramina/array_float_len.lawn", "r");
    Stream len_n = mk_stream_open_c("gramina/array_negative_len.lawn", "r");

    bool success_f = check_compilation_success(&len_f);
    bool success_n = check_compilation_success(&len_n);

    stream_free(&len_f);
    stream_free(&len_n);

    if (success_f || success_n) {
        test_fail();
    } else {
        test_ok();
    }
}

TEST(ArrayOk) {
    Stream source = mk_stream_open_c("gramina/array_ok.lawn", "r");
    bool success = check_compilation_success(&source);

    if (success) {
        test_ok();
    } else {
        test_fail();
    }
}
