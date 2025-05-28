#define GRAMINA_NO_NAMESPACE

#include <stdarg.h>

#include "common/array.h"
#include "common/log.h"

typedef struct {
    void *userdata;
    gramina_log_callback func;
} LogEntry;

typedef struct {
    LogLevel level;
} LogStreamInfo;

GRAMINA_DECLARE_ARRAY(LogEntry, static);
GRAMINA_IMPLEMENT_ARRAY(LogEntry, static);

static Array(LogEntry) entries = {};

LogLevel gramina_global_log_level = GRAMINA_LOG_LEVEL_WARN;
Stream *gramina_log_redirect = NULL;

void __gramina_log_cleanup() {
    array_free(LogEntry, &entries);
}

void gramina_register_log_callback(LogCallback func, void *userdata) {
    array_append(LogEntry, &entries, ((LogEntry) {
        .func = func,
        .userdata = userdata,
    }));
}

bool gramina_remove_log_callback(LogCallback func) {
    size_t remove_idx = (size_t)-1;

    array_foreach_ref(LogEntry, i, entry, entries) {
        if (entry->func == func) {
            remove_idx = i;
            break;
        }
    }

    if (remove_idx != (size_t)-1) {
        array_remove(LogEntry, &entries, remove_idx);
        return true;
    }

    return false;
}

static int log_stream_cleaner(Stream *this) {
    gramina_free(this->userdata);

    return 0;
}

static int log_stream_writer(Stream *this, const uint8_t *buf, size_t bufsize) {
    LogStreamInfo *info = this->userdata;
    log_fmt(info->level, "{svo}", mk_sv_buf(buf, bufsize));

    return 0;
}

Stream gramina_mk_stream_log(LogLevel level) {
    LogStreamInfo *info = gramina_malloc(sizeof *info);
    *info = (LogStreamInfo) {
        .level = level,
    };

    Stream base = mk_stream();
    base.userdata = info;
    base.cleaner = log_stream_cleaner,
    base.writer = log_stream_writer;

    return base;
}

void gramina_log_vsvfmt(LogLevel level, const StringView *fmt, va_list args) {
    if (level < gramina_global_log_level || level > GRAMINA_LOG_LEVEL_NONE) {
        return;
    }

    bool urgent = level == GRAMINA_LOG_LEVEL_NONE;

    String out = str_vfmt(fmt, args);

    FILE *stream;
    switch (level) {
    case GRAMINA_LOG_LEVEL_VERBOSE:
    case GRAMINA_LOG_LEVEL_INFO:
        stream = stdout;
        break;
    case GRAMINA_LOG_LEVEL_WARN:
    case GRAMINA_LOG_LEVEL_ERROR:
    case GRAMINA_LOG_LEVEL_NONE:
        stream = stderr;
        break;
    }

    const char *prefix;
    const char *color;
    switch (level) {
    case GRAMINA_LOG_LEVEL_VERBOSE:
        prefix = "V: ";
        color = "\x1b[90m";
        break;
    case GRAMINA_LOG_LEVEL_INFO:
        prefix = "I: ";
        color = "";
        break;
    case GRAMINA_LOG_LEVEL_WARN:
        prefix = "W: ";
        color = "\x1b[93m";
        break;
    case GRAMINA_LOG_LEVEL_ERROR:
        prefix = "E: ";
        color = "\x1b[91m";
        break;
    default:
        // If you log using the `NONE` level, it must be REALLY important
        prefix = "!! ";
        color = "\x1b[95m";
        break;
    }

    fprintf(stream, "%s\x1b[1m%s%s", color, prefix, urgent ? "" : "\x1b[22m");
    fwrite(out.data, 1, out.length, stream);
    fprintf(stream, "\x1b[0m");

    if (gramina_log_redirect) {
        stream_write_cstr(gramina_log_redirect, prefix);
        stream_write_str(gramina_log_redirect, &out);
    }

    array_foreach_ref(LogEntry, _, entry, entries) {
        StringView view = str_as_view(&out);
        entry->func(entry->userdata, level, &view);
    }

    str_free(&out);
}

void gramina_log_fmt(LogLevel level, const char *fmt, ...) {
    StringView sv = mk_sv_c(fmt);

    va_list args;
    va_start(args, fmt);

    log_vsvfmt(level, &sv, args);

    va_end(args);
}

void gramina_vlog_fmt(const char *fmt, ...) {
    StringView sv = mk_sv_c(fmt);

    va_list args;
    va_start(args, fmt);

    log_vsvfmt(GRAMINA_LOG_LEVEL_VERBOSE, &sv, args);

    va_end(args);
}

void gramina_ilog_fmt(const char *fmt, ...) {
    StringView sv = mk_sv_c(fmt);

    va_list args;
    va_start(args, fmt);

    log_vsvfmt(GRAMINA_LOG_LEVEL_INFO, &sv, args);

    va_end(args);
}

void gramina_wlog_fmt(const char *fmt, ...) {
    StringView sv = mk_sv_c(fmt);

    va_list args;
    va_start(args, fmt);

    log_vsvfmt(GRAMINA_LOG_LEVEL_WARN, &sv, args);

    va_end(args);
}

void gramina_elog_fmt(const char *fmt, ...) {
    StringView sv = mk_sv_c(fmt);

    va_list args;
    va_start(args, fmt);

    log_vsvfmt(GRAMINA_LOG_LEVEL_ERROR, &sv, args);

    va_end(args);
}
