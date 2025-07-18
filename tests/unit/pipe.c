#define GRAMINA_NO_NAMESPACE
#include "common/subprocess.h"

#include "tester.h"

#if defined(unix) || defined(__unix__) || defined(__unix)
#  define GRAMINA_UNIX_BUILD
#endif

TEST(Pipe) {
#ifdef GRAMINA_UNIX_BUILD
    Subprocess sbp = mk_sbp();
    sbp_arg_cstr(&sbp, "cat");
    sbp_run(&sbp);

    StringView payload = mk_sv_c("qazwsxedcrfvtgbyhn");
    Stream stream = sbp_stream(&sbp);

    int status = 0;
    status |= stream_write_sv(&stream, &payload);

    size_t read = 0;
    uint8_t buf[payload.length];
    status |= stream_read_buf(&stream, buf, sizeof buf, &read);

    StringView buf_view = mk_sv_buf(buf, sizeof buf);

    if (status || read != sizeof buf || sv_cmp(&payload, &buf_view) != 0) {
        stream_free(&stream);
        sbp_free(&sbp);
        test_fail();
    }

    stream_free(&stream);
    sbp_free(&sbp);

#else
    fprintf(stderr, "This test is disabled on non-UNIX environments\n");
#endif

    test_ok();
}
