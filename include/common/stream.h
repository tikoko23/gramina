#ifndef __GRAMINA_COMMON_STREAM
#define __GRAMINA_COMMON_STREAM

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "common/str.h"

// All functions in this header return 0 on success and `EOF` on EOF, including callbacks

struct gramina_stream;

typedef int (*gramina_stream_reader)(struct gramina_stream *this, uint8_t *buf, size_t bufsize, size_t *read);
typedef int (*gramina_stream_writer)(struct gramina_stream *this, const uint8_t *buf, size_t bufsize);
typedef int (*gramina_stream_flusher)(struct gramina_stream *this);
typedef int (*gramina_stream_cleaner)(struct gramina_stream *this); // Cleanup and dealloc
typedef bool (*gramina_stream_validator)(const struct gramina_stream *this);

struct gramina_stream {
    gramina_stream_reader reader;
    gramina_stream_writer writer;
    gramina_stream_flusher flusher;
    gramina_stream_cleaner cleaner;
    gramina_stream_validator validator;

    void *userdata;
    // TODO: proper buffering
    // size_t max_buffered_bytes;
    // struct gramina_string out_buffer;
};

struct gramina_stream gramina_mk_stream();
struct gramina_stream gramina_mk_stream_file(FILE *f, bool readable, bool writable);
struct gramina_stream gramina_mk_stream_open(struct gramina_string_view filename, struct gramina_string_view mode);
struct gramina_stream gramina_mk_stream_open_c(const char *filename, const char *mode);

struct gramina_stream gramina_mk_stream_str_own(struct gramina_string str, bool readable, bool writable);

void gramina_stream_shrink(struct gramina_stream *this);
void gramina_stream_free(struct gramina_stream *this);

bool gramina_stream_is_readable(const struct gramina_stream *this);
bool gramina_stream_is_writable(const struct gramina_stream *this);
bool gramina_stream_is_valid(const struct gramina_stream *this);

int gramina_stream_flush(struct gramina_stream *this);

int gramina_stream_write_sv(struct gramina_stream *this, const struct gramina_string_view *sv);
int gramina_stream_write_str(struct gramina_stream *this, const struct gramina_string *str);
int gramina_stream_write_cstr(struct gramina_stream *this, const char *cstr);
int gramina_stream_write_cfmt(struct gramina_stream *this, const char *fmt, ...);
int gramina_stream_write_buf(struct gramina_stream *this, const uint8_t *buf, size_t bufsize);
int gramina_stream_write_byte(struct gramina_stream *this, uint8_t byte);

// Appends at the end
int gramina_stream_read_str(struct gramina_stream *this, struct gramina_string *str, size_t max_bytes, size_t *read);
int gramina_stream_read_buf(struct gramina_stream *this, uint8_t *buf, size_t max_bytes, size_t *read);
int gramina_stream_read_byte(struct gramina_stream *this, uint8_t *byte);

bool gramina_stream_file_is_valid(struct gramina_stream *this);

#endif

#ifdef GRAMINA_NO_NAMESPACE

typedef gramina_stream_reader StreamReader;
typedef gramina_stream_writer StreamWriter;
typedef gramina_stream_flusher StreamFlusher;
typedef gramina_stream_cleaner StreamCleaner;

#endif
#ifdef GRAMINA_WANT_TAGLESS

typedef gramina_stream_reader GraminaStreamReader;
typedef gramina_stream_writer GraminaStreamWriter;
typedef gramina_stream_flusher GraminaStreamFlusher;
typedef gramina_stream_cleaner GraminaStreamCleaner;

#endif

#include "gen/common/stream.h"
