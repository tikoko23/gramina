#define GRAMINA_NO_NAMESPACE

#include <errno.h>
#include <string.h>

#include "common/log.h"
#include "common/subprocess.h"

GRAMINA_IMPLEMENT_ARRAY(_GraminaSbpArg);

#ifdef GRAMINA_UNIX_BUILD
#include <sys/wait.h>
#endif

#ifdef GRAMINA_UNIX_BUILD
static int summon_child(const Subprocess *this) {
    char *argv[this->argv.length + 1];

    for (size_t i = 0; i < this->argv.length; ++i) {
        argv[i] = str_to_cstr(this->argv.items + i);
    }

    argv[this->argv.length] = NULL;

    if (execvp(argv[0], argv) < 0) {
        for (size_t i = 0; i < this->argv.length; ++i) {
            gramina_free(argv[i]);
        }
    }

    elog_fmt("Failed to summon child process: {cstr}\n", strerror(errno));

    exit(-1); 
}
#endif

Subprocess gramina_mk_sbp() {
    return (Subprocess) {
        .argv = mk_array(_GraminaSbpArg),
    };
}

void gramina_sbp_arg_own(Subprocess *this, String arg) {
    array_append(_GraminaSbpArg, &this->argv, arg);
}

void gramina_sbp_arg_sv(Subprocess *this, const StringView *arg) {
    array_append(_GraminaSbpArg, &this->argv, sv_dup(arg));
}

void gramina_sbp_arg_cstr(Subprocess *this, const char *arg) {
    array_append(_GraminaSbpArg, &this->argv, mk_str_c(arg));
}

int gramina_sbp_run(Subprocess *this) {
    if (this->argv.length == 0) {
        return 1;
    }

#ifdef GRAMINA_UNIX_BUILD
    pid_t pid = fork();

    if (pid < 0) {
        return errno;
    }

    if (pid != 0) {
        this->handle = pid;
        return 0;
    } else {
        summon_child(this);
    }

#endif

    return 1;
}

int gramina_sbp_run_sync(Subprocess *this) {
    int status;
    if ((status = sbp_run(this))) {
        return status;
    }

    return sbp_wait(this);
}

int gramina_sbp_wait(Subprocess *this) {
#ifdef GRAMINA_UNIX_BUILD
    siginfo_t info;
    if (waitid(P_PID, this->handle, &info, WEXITED) < 0) {
        return errno;
    }

    this->exit_code = info.si_status;
#endif

    return 0;
}

long gramina_sbp_pid(const Subprocess *this) {
#ifdef GRAMINA_UNIX_BUILD
    return this->handle;
#endif

    return 0;
}

void gramina_sbp_free(Subprocess *this) {
    array_foreach_ref(_GraminaSbpArg, _, arg, this->argv) {
        str_free(arg);
    }

    array_free(_GraminaSbpArg, &this->argv);
}
