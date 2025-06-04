#define GRAMINA_NO_NAMESPACE

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "common/mem.h"
#include "common/stream.h"

struct __gramina_fs_data {
    FILE *file;
    bool owned;
    size_t read_pos;
};

typedef struct {
    String string;
    size_t read_idx;
} StringStreamData;

Stream gramina_mk_stream() {
    return (Stream) {
        .reader = NULL,
        .writer = NULL,
        .flusher = NULL,
        .cleaner = NULL,
        .validator = NULL,
        .userdata = NULL,
        // .max_buffered_bytes = 1024,
        // .out_buffer = mk_str(),
    };
}

static int __gramina_fs_reader(Stream *this, uint8_t *buf, size_t bufsize, size_t *read) {
    struct __gramina_fs_data *data = this->userdata;
    FILE *file = data->file;

    fseek(file, data->read_pos, SEEK_SET);

    size_t n_read = fread(buf, 1, bufsize, file);
    data->read_pos += n_read;

    if (read != NULL) {
        *read = n_read;
    }

    if (feof(file)) {
        return EOF;
    }

    if (ferror(file)) {
        return EIO;
    }

    return 0;
}

static int __gramina_fs_writer(Stream *this, const uint8_t *buf, size_t bufsize) {
    struct __gramina_fs_data *data = this->userdata;
    FILE *file = data->file;

    fseek(file, 0, SEEK_END);

    size_t written = fwrite(buf, 1, bufsize, file);
    if (feof(file)) {
        return EOF;
    }

    if (ferror(file) || written < bufsize) {
        return EIO;
    }

    return 0;
}

static int __gramina_fs_flusher(Stream *this) {
    struct __gramina_fs_data *data = this->userdata;
    fflush(data->file);

    return 0;
}

static int __gramina_fs_cleaner(Stream *this) {
    struct __gramina_fs_data *data = this->userdata;

    if (data->owned) {
        fclose(data->file);
    }

    gramina_free(this->userdata);
    this->userdata = NULL;

    return 0;
}

static bool __gramina_fs_validator(const Stream *this) {
    struct __gramina_fs_data *ud = this->userdata;

    return ud != NULL
        && ud->file != NULL
        && this->flusher == __gramina_fs_flusher
        && this->cleaner == __gramina_fs_cleaner
        && (this->reader == __gramina_fs_reader
         || this->writer == __gramina_fs_writer
        );
}

static int strs_reader(Stream *this, uint8_t *buf, size_t bufsize, size_t *read) {
    StringStreamData *data = this->userdata;

    size_t n_left = data->string.length - data->read_idx;
    size_t n_read = bufsize > n_left
                  ? n_left
                  : bufsize;

    memcpy(buf, data->string.data + data->read_idx, n_read);

    if (read) {
        *read = n_read;
    }

    data->read_idx += n_read;

    if (data->read_idx >= data->string.length) {
        return EOF;
    }

    return 0;
}

static int strs_writer(Stream *this, const uint8_t *buf, size_t bufsize) {
    StringStreamData *data = this->userdata;

    StringView to_be_written = mk_sv_buf(buf, bufsize);

    str_cat_sv(&data->string, &to_be_written);

    return 0;
}

static int strs_cleaner(Stream *this) {
    StringStreamData *data = this->userdata;
    str_free(&data->string);
    gramina_free(data);

    return 0;
}

// There isn't really a proper way to check validity
static bool strs_validator(const Stream *this) {
    return true;
}

Stream gramina_mk_stream_file(FILE *f, bool readable, bool writable) {
    Stream this = mk_stream();

    this.reader = readable ? __gramina_fs_reader : NULL;
    this.writer = writable ? __gramina_fs_writer : NULL;
    this.flusher = __gramina_fs_flusher;
    this.cleaner = __gramina_fs_cleaner;
    this.validator = __gramina_fs_validator;

    struct __gramina_fs_data *userdata = gramina_malloc(sizeof (struct __gramina_fs_data));
    *userdata = (struct __gramina_fs_data) {
        .file = f,
        .owned = false,
        .read_pos = 0,
    };

    this.userdata = userdata;

    return this;
}

Stream gramina_mk_stream_open(StringView filename, StringView mode) {
    bool readable = false;
    bool writable = false;
    sv_foreach(_, c, mode) {
        switch (c) {
        case 'r':
            readable = true;
            break;
        case 'w':
            writable = true;
            break;
        }
    }

    String c_name = sv_dup(&filename);
    String c_mode = sv_dup(&mode);

    str_append(&c_name, '\0');
    str_append(&c_mode, '\0');

    FILE *file = fopen(c_name.data, c_mode.data);

    str_free(&c_name);
    str_free(&c_mode);

    if (!file) {
        return (Stream) {
            .validator = __gramina_fs_validator,
            .userdata = NULL,
        };
    }

    Stream this = gramina_mk_stream_file(file, readable, writable);
    struct __gramina_fs_data *userdata = this.userdata;
    userdata->owned = true;

    return this;
}

Stream gramina_mk_stream_open_c(const char *filename, const char *mode) {
    StringView n = mk_sv_c(filename);
    StringView m = mk_sv_c(mode);

    return mk_stream_open(n, m);
}

Stream gramina_mk_stream_str_own(String str, bool readable, bool writable) {
    Stream this = mk_stream();

    this.reader = readable ? strs_reader : NULL;
    this.writer = writable ? strs_writer : NULL;
    this.cleaner = strs_cleaner;
    this.validator = strs_validator;

    StringStreamData *userdata = gramina_malloc(sizeof *userdata);
    *userdata = (StringStreamData) {
        .string = str,
        .read_idx = 0,
    };

    this.userdata = userdata;

    return this;
}

void gramina_stream_shrink(Stream *this) {
    // str_shrink(&this->in_buffer);
    // str_shrink(&this->out_buffer);
}

void gramina_stream_free(Stream *this) {
    stream_flush(this);

    if (this->cleaner != NULL) {
        this->cleaner(this);
    }

    // str_free(&this->in_buffer);
    // str_free(&this->out_buffer);
}

bool gramina_stream_is_readable(const Stream *this) {
    return this->reader != NULL;
}

bool gramina_stream_is_writable(const Stream *this) {
    return this->writer != NULL;
}

bool gramina_stream_is_valid(const Stream *this) {
    if (!this->validator) {
        return this->reader || this->writer;
    }

    return this->validator(this);
}

int gramina_stream_flush(Stream *this) {
    /*
    if (stream_is_writable(this)) {
        this->writer(
            this,
            (const unsigned char *)this->out_buffer.data,
            this->out_buffer.length
        );
    }
    */

    if (this->flusher != NULL) {
        return this->flusher(this);
    }

    return 0;
}

int gramina_stream_write_sv(Stream *this, const StringView *sv) {
    return stream_write_buf(this, (const unsigned char *)sv->data, sv->length);
}

int gramina_stream_write_str(Stream *this, const String *str) {
    return stream_write_buf(this, (const unsigned char *)str->data, str->length);
}

int gramina_stream_write_cstr(Stream *this, const char *cstr) {
    size_t len = strlen(cstr);
    return stream_write_buf(this, (const unsigned char *)cstr, len);
}

int gramina_stream_write_cfmt(Stream *this, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    String out = str_vcfmt(fmt, args);
    int status = stream_write_str(this, &out);
    str_free(&out);

    va_end(args);
    return status;
}

int gramina_stream_write_buf(Stream *this, const uint8_t *buf, size_t bufsize) {
    if (!stream_is_writable(this)) {
        return EINVAL;
    }

    /*
    if (this->out_buffer.length >= this->max_buffered_bytes) {
        int status = stream_flush(this);
        if (status) {
            return status;
        }

        str_clear(&this->out_buffer);
    }

    size_t available = this->max_buffered_bytes - this->out_buffer.length;
    if (available >= bufsize) {
        int status = 0;
        StringView data = mk_sv_buf(buf, bufsize);
        str_cat_sv(&this->out_buffer, &data);

        if (available == bufsize) {
            status = stream_flush(this);
        }

        return status;
    }

    int status = stream_flush(this);
    if (status) {
        return status;
    }
    */

    return this->writer(this, buf, bufsize);
}

int gramina_stream_write_byte(struct gramina_stream *this, uint8_t byte) {
    return stream_write_buf(this, &byte, 1);
}

int gramina_stream_read_str(Stream *this, String *str, size_t max_bytes, size_t *read) {
    if (!stream_is_readable(this)) {
        return EINVAL;
    }

    size_t __read;
    if (read == NULL) {
        read = &__read;
    }

    *read = 0;

    int64_t read_left = max_bytes;
    while (read_left > 0) {
        size_t bufsize = 128;
        if (read_left < 128) {
            bufsize = read_left;
        }

        str_reserve(str, str->length + bufsize);
        size_t buf_idx = str->length;

        for (size_t i = 0; i < bufsize; ++i) {
            str_append(str, '\0');
        }

        size_t r;
        int status = this->reader(this, (uint8_t *)str->data + buf_idx, bufsize, &r);

        for (size_t i = 0; i < (bufsize - r); ++i) {
            str_pop(str);
        }

        *read += r;
        read_left -= bufsize;

        if (status) {
            return status;
        }
    }

    return 0;
}

int gramina_stream_read_buf(Stream *this, uint8_t *buf, size_t max_bytes, size_t *read) {
    if (!stream_is_readable(this)) {
        return EINVAL;
    }

    return this->reader(this, buf, max_bytes, read);
}

int gramina_stream_read_byte(struct gramina_stream *this, uint8_t *byte) {
    return stream_read_buf(this, byte, 1, NULL);
}

// Old API, will be deprecated
bool gramina_stream_file_is_valid(Stream *this) {
    return __gramina_fs_validator(this);
}
