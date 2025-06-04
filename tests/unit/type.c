#include "tester.h"

#include "compiler/type.h"

static void test_primitives() {
    Type t = type_from_primitive(GRAMINA_PRIMITIVE_BOOL);
    String s = type_to_str(&t);

    if (str_cmp_c(&s, "bool")) {
        str_free(&s);
        test_fail();
    }

    str_free(&s);

    t.is_const = true;
    s = type_to_str(&t);

    if (str_cmp_c(&s, "const bool")) {
        str_free(&s);
        test_fail();
    }

    str_free(&s);
}

void TEST_TypeToString() {
    test_primitives();
    test_ok();
}

TEST(Constness) {
    Stream const_assign = mk_stream_open_c("gramina/const_assign.lawn", "r");
    Stream const_pointer_arg = mk_stream_open_c("gramina/const_ptr_arg.lawn", "r");
    Stream const_pointer_bad_init = mk_stream_open_c("gramina/const_ptr_bad_init.lawn", "r");
    Stream const_pointer_bad_assign = mk_stream_open_c("gramina/const_ptr_bad_assign.lawn", "r");

    Stream *tus[] = {
        &const_assign,
        &const_pointer_arg,
        &const_pointer_bad_init,
        &const_pointer_bad_assign,
    };

    bool failed = false;

    for (size_t i = 0; i < (sizeof tus) / (sizeof tus[i]); ++i) {
        // None of these should compile
        if (check_compilation_success(tus[i])) {
            failed = true;
            break;
        }
    }

    for (size_t i = 0; i < (sizeof tus) / (sizeof tus[i]); ++i) {
        stream_free(tus[i]);
    }

    if (failed) {
        test_fail();
    }

    test_ok();
}
