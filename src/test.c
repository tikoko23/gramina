/**
 * !!!!
 * This file will soon be moved into a seperate module for the gramina-cli
 * !!!!
 */

#define GRAMINA_NO_NAMESPACE

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <llvm-c/Core.h>
#include <llvm-c/Error.h>

#include "cli/help.h"

#include "common/arg.h"
#include "common/array.h"
#include "common/error.h"
#include "common/hashmap.h"
#include "common/init.h"
#include "common/log.h"

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

int _main(int argc, char **argv) {
    init();
    atexit(cleanup);

    enum {
        HELP_ARG,
        VERBOSE_ARG,
        OUT_FILE_ARG,
        LOG_LEVEL_ARG,
        AST_DUMP_ARG,
        LINK_LIB_ARG,
    };

    Array(_GraminaArgString) linked_libs = mk_array(_GraminaArgString);

    ArgumentInfo argument_info[] = {
        [HELP_ARG] = {
            .name = "help",
            .type = GRAMINA_ARG_LONG,
            .param_needs = GRAMINA_PARAM_OPTIONAL,
            .override_behavior = GRAMINA_OVERRIDE_FORBID,
        },
        [VERBOSE_ARG] = {
            .name = "verbose",
            .flag = 'v',
            .type = GRAMINA_ARG_LONG | GRAMINA_ARG_FLAG,
            .param_needs = GRAMINA_PARAM_NONE,
            .override_behavior = GRAMINA_OVERRIDE_OK,
        },
        [OUT_FILE_ARG] = {
            .flag = 'o',
            .type = GRAMINA_ARG_FLAG,
            .param_needs = GRAMINA_PARAM_REQUIRED,
            .override_behavior = GRAMINA_OVERRIDE_FORBID,
        },
        [LOG_LEVEL_ARG] = {
            .name = "log-level",
            .type = GRAMINA_ARG_LONG,
            .param_needs = GRAMINA_PARAM_REQUIRED,
            .override_behavior = GRAMINA_OVERRIDE_FORBID,
        },
        [AST_DUMP_ARG] = {
            .name = "ast-dump",
            .type = GRAMINA_ARG_LONG,
            .param_needs = GRAMINA_PARAM_REQUIRED,
            .override_behavior = GRAMINA_OVERRIDE_FORBID,
        },
        [LINK_LIB_ARG] = {
            .flag = 'l',
            .type = GRAMINA_ARG_FLAG,
            .param_needs = GRAMINA_PARAM_MULTI,
            .override_behavior = GRAMINA_OVERRIDE_OK,
            .multi_params = &linked_libs,
        }
    };

    Arguments args = {
        .named = mk_array_c(_GraminaArgInfo, argument_info, (sizeof argument_info) / (sizeof argument_info[0])),
    };

    if (args_parse(&args, argc, argv)) {
        elog_fmt("Arguments: {s}\n", &args.error);
        args_free(&args);
        return -1;
    }

    ArgumentInfo *help_arg = &args.named.items[HELP_ARG];

    if (help_arg->found) {
        if (!help_arg->param) {
            cli_show_general_help();
        } else {
            cli_show_specific_help(help_arg->param);
        }

        args_free(&args);
        return 0;
    }

    if (args.positional.length == 0) {
        elog_fmt("Missing file argument\n");
        args_free(&args);
        return -1;
    }

    ArgumentInfo *outfile_arg = &args.named.items[OUT_FILE_ARG];
    ArgumentInfo *log_level_arg = &args.named.items[LOG_LEVEL_ARG];
    ArgumentInfo *verbose_arg = &args.named.items[VERBOSE_ARG];
    ArgumentInfo *ast_dump_arg = &args.named.items[AST_DUMP_ARG];

    const char *source = args.positional.items[0];
    const char *outfile = outfile_arg->found
                        ? outfile_arg->param
                        : "out.ll";

    size_t excess_file_args = args.positional.length - 1;

    if (excess_file_args) {
        wlog_fmt("Ignoring {sz} extra file argument{cstr} provided\n", excess_file_args, excess_file_args == 1 ? "" : "s");
    }

    if (verbose_arg->found) {
        gramina_global_log_level = GRAMINA_LOG_LEVEL_VERBOSE;
    }

    if (log_level_arg->found) {
        StringView param = mk_sv_c(log_level_arg->param);

        if (false) {
        } else if (sv_cmp_c(&param, "all") == 0) {
            gramina_global_log_level = GRAMINA_LOG_LEVEL_VERBOSE;
        } else if (sv_cmp_c(&param, "info") == 0) {
            gramina_global_log_level = GRAMINA_LOG_LEVEL_INFO;
        } else if (sv_cmp_c(&param, "warn") == 0) {
            gramina_global_log_level = GRAMINA_LOG_LEVEL_WARN;
        } else if (sv_cmp_c(&param, "error") == 0) {
            gramina_global_log_level = GRAMINA_LOG_LEVEL_ERROR;
        } else if (sv_cmp_c(&param, "silent") == 0) {
            gramina_global_log_level = GRAMINA_LOG_LEVEL_NONE;
        } else if (sv_cmp_c(&param, "none") == 0) {
            gramina_global_log_level = GRAMINA_LOG_LEVEL_NONE + 1;
        } else {
            elog_fmt("Unknown log level '{sv}'\n", &param);
            args_free(&args);
            return -1;
        }

        if (verbose_arg->found) {
            wlog_fmt("'--log-level' will override '--verbose' and '-v'\n");
        }
    }

    int status = 0;

    Stream file = mk_stream_open_c(source, "r");
    if (!stream_file_is_valid(&file)) {
        elog_fmt("Cannot open file '{cstr}': {cstr}\n", source, strerror(errno));
        array_free(_GraminaArgString, &linked_libs);
        args_free(&args);
        return -1;
    }

    Stream verbose_printer = mk_stream_log(GRAMINA_LOG_LEVEL_VERBOSE);
    gramina_assert(stream_is_valid(&verbose_printer));

    ilog_fmt("Entering the lexer phase...\n");

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
        stream_free(&verbose_printer);
        lex_result_free(&lex_result);
        array_free(_GraminaArgString, &linked_libs);
        args_free(&args);

        return status;
    }

    Slice(GraminaToken) tokens_s = array_as_slice(GraminaToken, &lex_result.tokens);

    slice_foreach(GraminaToken, _, t, tokens_s) {
        StringView rule = token_type_to_str(t.type);
        vlog_fmt(
            "position ({cpf:%2zu}, {cpf:%2zu}), rule: {sv}, value: '{so}'\n",
            t.pos.line,
            t.pos.column,
            &rule,
            token_contents(&t)
        );
    }

    ilog_fmt("Entering the parsing phase...\n");

    ParseResult presult = parse(&tokens_s);
    AstNode *root = presult.root;

    if (!root) {
        elog_fmt(
            "({sz}:{sz}) {s}\n",
            presult.error.pos.line,
            presult.error.pos.column,
            &presult.error.description
        );

        highlight_char(source, presult.error.pos.line, presult.error.pos.column);

        status = 1;
        goto _cleanup;
    }

    stream_write_cstr(&verbose_printer, "AST dump:\n");
    ast_print(root, &verbose_printer);

    if (ast_dump_arg->found) {
        Stream file = mk_stream_open_c(ast_dump_arg->param, "w");
        ast_print(root, &file);
        stream_flush(&file);
        stream_free(&file);
    }

    ilog_fmt("Entering the compilation phase...\n");

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

        goto _cleanup;
    }

    ilog_fmt("Compilation finished!\n");

    char *err;
    if (LLVMPrintModuleToFile(cresult.module, outfile, &err)) {
        elog_fmt("Error while writing '{cstr}': {cstr}\n", outfile, err);
        LLVMDisposeErrorMessage(err);
    }

    _cleanup:

    lex_result_free(&lex_result);
    parse_result_free(&presult);

    stream_free(&file);
    stream_free(&verbose_printer);

    LLVMDisposeModule(cresult.module);

    array_free(_GraminaArgString, &linked_libs);
    args_free(&args);

    return status;
}

