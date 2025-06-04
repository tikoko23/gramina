#include <string.h>

#include "tester.h"

#include "common/stream.h"

TEST(SStream) {
    Stream sstr = mk_stream_str_own(mk_str(), true, true);

    stream_write_cstr(&sstr, "orz");

    size_t n_read;
    uint8_t buf[4];
    stream_read_buf(&sstr, buf, 3, &n_read);

    buf[3] = '\0';

    if (strcmp((char *)buf, "orz")) {
        stream_free(&sstr);
        test_fail();
    }

    stream_write_cfmt(&sstr, "{i32}", (int32_t)23);

    String out = mk_str();
    stream_read_str(&sstr, &out, 5, NULL);

    if (str_cmp_c(&out, "23")) {
        str_free(&out);
        stream_free(&sstr);
        test_fail();
    }

    str_free(&out);
    stream_free(&sstr);

    test_ok();
}
