#include <stdint.h>
#include <unistd.h>
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
#define READ_END 0
#define WRITE_END 1

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

static int split(Subprocess *this) {
    pipe(this->pipe_in);
    pipe(this->pipe_out);

    pid_t pid = fork();

    if (pid < 0) {
        return errno;
    }

    if (pid != 0) {
        close(this->pipe_in[READ_END]);
        close(this->pipe_out[WRITE_END]);

        this->handle = pid;
        return 0;
    } else {
        close(this->pipe_in[WRITE_END]);
        close(this->pipe_out[READ_END]);

        dup2(this->pipe_in[READ_END], STDIN_FILENO);
        dup2(this->pipe_out[WRITE_END], STDOUT_FILENO);

        summon_child(this);
    }

    return 0;
}

static int child_read(Stream *this, uint8_t *buf, size_t bufsize, size_t *_read) {
    Subprocess *sbp = this->userdata;
    ssize_t status = read(sbp->pipe_out[READ_END], buf, bufsize);

    switch (status) {
    case -1:
        return errno;
    case 0:
        return EOF;
    default:
        if (_read) {
            *_read = status;
        }
    }

    return 0;
}

static int child_write(Stream *this, const uint8_t *buf, size_t bufsize) {
    Subprocess *sbp = this->userdata;
    ssize_t status = write(sbp->pipe_in[WRITE_END], buf, bufsize);

    if (status < 0) {
        return errno;
    }

    return 0;
}

#endif

Subprocess gramina_mk_sbp() {
    return (Subprocess) {
        .argv = mk_array(_GraminaSbpArg),
#ifdef GRAMINA_UNIX_BUILD
        .pipe_in = { -1, -1 },
        .pipe_out = { -1, -1 },
#endif
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
    return split(this);
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

struct gramina_stream gramina_sbp_stream(Subprocess *this) {
    Stream stream = {
#ifdef GRAMINA_UNIX_BUILD
        .reader = child_read,
        .writer = child_write,
        .userdata = this,
#endif
    };

    return stream;
}

void gramina_sbp_free(Subprocess *this) {
    array_foreach_ref(_GraminaSbpArg, _, arg, this->argv) {
        str_free(arg);
    }

    array_free(_GraminaSbpArg, &this->argv);

#ifdef GRAMINA_UNIX_BUILD
    if (this->pipe_in[WRITE_END] >= 0) {
        close(this->pipe_in[WRITE_END]);
    }

    if (this->pipe_out[READ_END] >= 0) {
        close(this->pipe_out[READ_END]);
    }
#endif
}
