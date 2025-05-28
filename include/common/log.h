#ifndef __GRAMINA_COMMON_LOG_H
#define __GRAMINA_COMMON_LOG_H

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include "str.h"
#include "stream.h"

/**
 * If disabling the forced logs is needed, do the following:
 *     gramina_global_log_level = GRAMINA_LOG_LEVEL_NONE + 1;
 */
enum gramina_log_level {
    GRAMINA_LOG_LEVEL_VERBOSE = 0,
    GRAMINA_LOG_LEVEL_INFO,
    GRAMINA_LOG_LEVEL_WARN,
    GRAMINA_LOG_LEVEL_ERROR,
    GRAMINA_LOG_LEVEL_NONE,
};

typedef void (*gramina_log_callback)(
    void *userdata,
    enum gramina_log_level level,
    const struct gramina_string_view *message
);

extern enum gramina_log_level gramina_global_log_level;
extern struct gramina_stream *gramina_log_redirect;

void gramina_log_vsvfmt(enum gramina_log_level level, const struct gramina_string_view *fmt, va_list args);
void gramina_log_fmt(enum gramina_log_level level, const char *fmt, ...);

void gramina_vlog_fmt(const char *fmt, ...); // VERBOSE
void gramina_ilog_fmt(const char *fmt, ...); // INFO
void gramina_wlog_fmt(const char *fmt, ...); // WARN
void gramina_elog_fmt(const char *fmt, ...); // ERROR

void gramina_register_log_callback(gramina_log_callback func, void *userdata);
bool gramina_remove_log_callback(gramina_log_callback func);

struct gramina_stream gramina_mk_stream_log(enum gramina_log_level level);

void __gramina_log_cleanup(); /* ignore */

#endif

#ifdef GRAMINA_NO_NAMESPACE
   typedef gramina_log_callback LogCallback;
#endif
#ifdef GRAMINA_WANT_TAGLESS
   typedef gramina_log_callback GraminaLogCallback;
#endif

#include "gen/common/log.h"
