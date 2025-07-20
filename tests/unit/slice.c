#include <stdint.h>

#include "tester.h"

TEST(SliceRef) {
    Subprocess compiler = mk_sbp();
    sbp_arg_cstr(&compiler, get_compiler());
    sbp_arg_cstr(&compiler, "gramina/slice_ref.lawn");
    set_compilation_output(&compiler, "local/slice_ref");
    add_libc(&compiler);

    sbp_run_sync(&compiler);
    if (compiler.exit_code) {
        sbp_free(&compiler);
        test_fail();
    }

    sbp_free(&compiler);

    bool fail = false; 
    execute("local/slice_ref", prog) {
        Stream stream = sbp_stream(&prog);
        String output = mk_str();
        int status = stream_read_str(&stream, &output, 1024, NULL);
        if (status && status != EOF) {
            fail = true;
            goto end;
        }

        str_append(&output, '\0');

        uint64_t len, ptr1, ptr2;
        sscanf(output.data, "%lu\n%lu\n%lu", &len, &ptr1, &ptr2);

        str_free(&output);

        if (len != 12 || ptr1 != ptr2) {
            fail = true;
            goto end;
        }

        end:
        sbp_wait(&prog);
    }

    if (fail) {
        test_fail();
    } else {
        test_ok();
    }
}
