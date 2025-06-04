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
