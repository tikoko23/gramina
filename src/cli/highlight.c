#define GRAMINA_NO_NAMESPACE

#include <stdio.h>

#include "common/error.h"

#include "cli/highlight.h"

void cli_highlight_char(const char *source, size_t line, size_t column) {
    FILE *file = fopen(source, "r");
    if (!file) {
        return;
    }

    HighlightInfo hi = {
        .enable = mk_str_c("\x1b[1;97;41m"),
        .disable = mk_str_c("\x1b[0m"),
        .underline = mk_str_c("\x1b[1;91m"),
    };

    HighlightPosition pos = {
        .start_line = line > 4
                    ? line - 4
                    : 1,

        .end_line = line,
        .start_column = column,
        .end_column = column,
    };

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    String contents = mk_str_cap(file_size);
    fread(contents.data, 1, file_size, file);
    contents.length = file_size;

    StringView contents_view = str_as_view(&contents);
    String expanded = str_expand_tabs(&contents_view, 4);
    StringView expanded_view = str_as_view(&expanded);

    String error_msg = highlight_line(&expanded_view, pos, &hi);
    fprintf(stderr, "%.*s\n", (int)error_msg.length, error_msg.data);

    str_free(&hi.enable);
    str_free(&hi.disable);
    str_free(&hi.underline);
    str_free(&contents);
    str_free(&expanded);
    str_free(&error_msg);
}
