#pragma once

/**
 * `#pragma once` is specifically avoided on regular headers since the codebase
 * heavily relies on the preprocessor, thus needs better control over it. In this
 * case, however, it is not necessary. For tests `#pragma once` is acceptable
 */

#ifndef TESTER_NO_AUTO_NAMESPACE
#  define GRAMINA_NO_NAMESPACE
#endif

#define MAKE_TEST(_name) ((Test){ .name = #_name, .tester = TEST_ ## _name })
#define TEST(name) void TEST_ ## name()

#include <setjmp.h>

#include "common/str.h"
#include "common/stream.h"
#include "common/subprocess.h"

enum {
    TEST_SUCCESS = 1,
    TEST_FAIL,
};

typedef void (*Tester)(void);

typedef struct {
    const char *name;
    Tester tester;
} Test;

extern jmp_buf tester_ret;

void test_ok();

void test_fail();
void test_fail_cmsg(const char *msg);
void test_fail_msg(String msg);

void run_command(Subprocess *sp);

void set_compilation_output(Subprocess *sp, const char *out);
void add_libc(Subprocess *sp);

const char *get_compiler();

bool check_compilation_success(Stream *source);
bool check_compilation_success_sv(const StringView *source);
bool check_compilation_success_cstr(const char *source);
