#include <stdlib.h>

#include "tester.h"

#include "cli/tu.h"
#include "common/log.h"

#include "parser/lexer.h"
#include "parser/parser.h"

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

bool check_compilation_success(Stream *source) {
    LogLevel old = gramina_global_log_level;
    gramina_global_log_level = GRAMINA_LOG_LEVEL_NONE;

    LexResult lres = lex(source);
    if (lres.status != GRAMINA_LEX_ERR_NONE) {
        lex_result_free(&lres);
        gramina_global_log_level = old;
        return false;
    }

    Slice(GraminaToken) tokens = array_as_slice(GraminaToken, &lres.tokens);
    ParseResult pres = parse(&tokens);
    if (!pres.root) {
        lex_result_free(&lres);
        parse_result_free(&pres);
        gramina_global_log_level = old;
        return false;
    }

    CompileResult cres = compile(pres.root);
    if (cres.status != GRAMINA_COMPILE_ERR_NONE) {
        lex_result_free(&lres);
        parse_result_free(&pres);
        str_free(&cres.error.description);
        gramina_global_log_level = old;
        return false;
    }

    lex_result_free(&lres);
    parse_result_free(&pres);

    gramina_global_log_level = old;
    return true;
}

bool check_compilation_success_sv(const StringView *_source) {
    Stream source = mk_stream_str_own(sv_dup(_source), true, false);

    bool ret = check_compilation_success(&source);
    stream_free(&source);

    return ret;
}

bool check_compilation_success_cstr(const char *source) {
    StringView sv = mk_sv_c(source);

    return check_compilation_success_sv(&sv);
}
