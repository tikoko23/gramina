#ifndef __GRAMINA_COMMON_SUBPROCESS_H
#define __GRAMINA_COMMON_SUBPROCESS_H

#include "common/def.h"

#ifdef GRAMINA_UNIX_BUILD
#  include <unistd.h>
#endif

#include "common/array.h"
#include "common/str.h"

typedef struct gramina_string _GraminaSbpArg;

GRAMINA_DECLARE_ARRAY(_GraminaSbpArg);

struct gramina_subprocess {
    struct gramina_array(_GraminaSbpArg) argv;

#ifdef GRAMINA_UNIX_BUILD 
    pid_t handle;
#endif

    int exit_code;
};

struct gramina_subprocess gramina_mk_sbp();
void gramina_sbp_arg_own(struct gramina_subprocess *this, struct gramina_string arg);
void gramina_sbp_arg_sv(struct gramina_subprocess *this, const struct gramina_string_view *arg);
void gramina_sbp_arg_cstr(struct gramina_subprocess *this, const char *arg);

int gramina_sbp_run(struct gramina_subprocess *this);
int gramina_sbp_run_sync(struct gramina_subprocess *this);
int gramina_sbp_wait(struct gramina_subprocess *this);

long gramina_sbp_pid(const struct gramina_subprocess *this);

void gramina_sbp_free(struct gramina_subprocess *this);

#endif
#include "gen/common/subprocess.h"
