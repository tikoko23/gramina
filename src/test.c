#define GRAMINA_NO_NAMESPACE

#include "common/array.h"
#include "common/error.h"
#include "common/hashmap.h"

#include "compiler/compiler.h"

#include "parser/ast.h"
#include "parser/lexer.h"
#include "parser/parser.h"

static void highlight_char(const char *source, size_t line, size_t column) {
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

int main(int argc, char **argv) {
    if (argc <= 1) {
        fprintf(stderr, "gramina-test: missing file argument\n");
        return -1;
    }

    char *source = argv[1];

    int status = 0;

    Stream file = mk_stream_open_c(source, "r");
    gramina_assert(stream_file_is_valid(&file));

    Stream printer = mk_stream_file(stdout, false, true);
    gramina_assert(stream_file_is_valid(&printer));

    puts("==== LEXER BEGIN ====");

    LexResult lex_result = lex(&file);
    if (lex_result.status != GRAMINA_LEX_ERR_NONE) {
        StringView err_type = lex_error_code_to_str(lex_result.status);

        fprintf(
            stderr,
            "(%zu:%zu) %.*s: %.*s\n",
            lex_result.error_position.line,
            lex_result.error_position.column,
            (int)err_type.length, err_type.data,
            (int)lex_result.error_description.length, lex_result.error_description.data
        );

        highlight_char(source, lex_result.error_position.line, lex_result.error_position.column);

        status = lex_result.status;

        stream_free(&file);
        stream_free(&printer);
        lex_result_free(&lex_result);
        return status;
    }

    Slice(GraminaToken) tokens_s = array_as_slice(GraminaToken, &lex_result.tokens);

    slice_foreach(GraminaToken, _, t, tokens_s) {
        StringView rule = token_type_to_str(t.type);
        String contents = token_contents(&t);
        printf(
            "position (%2zu, %2zu), rule: %.*s, value: '%.*s'\n",
            t.pos.line,
            t.pos.column,
            (int)rule.length,
            rule.data,
            (int)contents.length,
            contents.data
        );

        str_free(&contents);
    }

    puts("==== LEXER END ====");
    puts("==== PARSER BEGIN ====");

    ParseResult presult = parse(&tokens_s);
    AstNode *root = presult.root;

    if (!root) {
        fprintf(
            stderr,
            "(%zu:%zu) %.*s\n",
            presult.error.pos.line,
            presult.error.pos.column,
            (int)presult.error.description.length,
            presult.error.description.data
        );

        highlight_char(source, presult.error.pos.line, presult.error.pos.column);

        status = 1;
        goto cleanup;
    }

    ast_print(root, &printer);
    puts("==== PARSER END ====");

    puts("==== COMPILER BEGIN ====");
    CompileResult cresult = compile(root);
    if (cresult.status) {
        CompileError *err = &cresult.error;
        StringView stat = compile_error_code_to_str(cresult.status);
        fprintf(
            stderr,
            "Status: %d (%.*s)\n(%zu:%zu) %.*s\n",
            cresult.status,
            (int)stat.length, stat.data,
            err->pos.line,
            err->pos.column,
            (int)err->description.length,
            err->description.data
        );

        highlight_char(source, cresult.error.pos.line, cresult.error.pos.column);

        status = 1;

        str_free(&cresult.error.description);
    }

    puts("==== COMPILER END ====");

    cleanup:

    lex_result_free(&lex_result);
    parse_result_free(&presult);

    stream_free(&file);
    stream_free(&printer);

    return status;
}

