#include <llvm-c/Error.h>
#define GRAMINA_NO_NAMESPACE

#include <errno.h>
#include <string.h>

#include <llvm-c/Core.h>

#include "cli/highlight.h"
#include "cli/state.h"
#include "cli/tu.h"

#include "common/log.h"

GRAMINA_IMPLEMENT_ARRAY(TranslationUnit);
GRAMINA_IMPLEMENT_ARRAY(CompilationStage);

static const char *get_name(CompilationStageProcessor p) {
    if (false) {
    } else if (p == tu_load) {
        return "LOAD";
    } else if (p == tu_lex) {
        return "LEX";
    } else if (p == tu_parse) {
        return "PARSE";
    } else if (p == tu_ast_dump) {
        return "AST_DUMP";
    } else if (p == tu_ast_log) {
        return "AST_LOG";
    } else if (p == tu_compile) {
        return "COMPILE";
    } else if (p == tu_ir_dump) {
        return "IR_DUMP";
    }

    return "UNKNOWN";
}

Pipeline pipeline_default(CliState *S) {
    Pipeline P = {
        .stages = mk_array(CompilationStage),
    };

    CompilationStageProcessor stages[] = {
        tu_load,
        tu_lex,
        tu_parse,
        S->ast_dump_file
            ? tu_ast_dump
            : NULL,
        gramina_global_log_level == GRAMINA_LOG_LEVEL_VERBOSE
            ? tu_ast_log
            : NULL,
        tu_compile,
        tu_ir_dump,
    };

    for (size_t i = 0; i < (sizeof stages) / (sizeof stages[0]); ++i) {
        CompilationStageProcessor proc = stages[i];
        if (!proc) {
            continue;
        }

        array_append(CompilationStage, &P.stages, ((CompilationStage) {
            .name = get_name(proc),
            .processor = proc,
        }));
    }

    return P;
}

void pipeline_free(Pipeline *this) {
    array_free(CompilationStage, &this->stages);
}

bool tu_pipe(CliState *S, const Pipeline *P, TranslationUnit *T) {
    array_foreach_ref(CompilationStage, _, stage, P->stages) {
        if (!stage->name) {
            stage->name = "<unknown>";
        }

        ilog_fmt("{cstr}: Stage '{cstr}'\n", T->file, stage->name);

        if (stage->processor(S, T)) {
            elog_fmt("{cstr}: Stage '{cstr}' failed\n", T->file, stage->name);
            return true;
        }
    }

    return false;
}

bool tu_load(CliState *S, TranslationUnit *T) {
    T->fstream = mk_stream_open_c(T->file, "r");

    if (!stream_is_valid(&T->fstream)) {
        elog_fmt("Cannot open file '{cstr}': {cstr}\n", T->file, strerror(errno));
        return true;
    }

    return false;
}

bool tu_lex(CliState *S, TranslationUnit *T) {
    T->lex_result = lex(&T->fstream);

    if (T->lex_result.status != GRAMINA_LEX_ERR_NONE) {
        StringView err_type = lex_error_code_to_str(T->lex_result.status);
        TokenPosition pos = T->lex_result.error_position;

        elog_fmt(
            "{cstr} ({sz}:{sz}) {sv}: {s}\n",
            T->file,
            pos.line, pos.column,
            &err_type,
            &T->lex_result.error_description
        );

        cli_highlight_char(T->file, pos.line, pos.column);
        return true;
    }

    return false;
}

bool tu_parse(CliState *S, TranslationUnit *T) {
    Slice(GraminaToken) tokens = array_as_slice(GraminaToken, &T->lex_result.tokens);
    T->parse_result = parse(&tokens);

    if (!T->parse_result.root) {
        TokenPosition pos = T->parse_result.error.pos;

        elog_fmt(
            "{cstr} ({sz}:{sz}) {s}\n",
            T->file,
            T->parse_result.error.pos.line,
            T->parse_result.error.pos.column,
            &T->parse_result.error.description
        );

        cli_highlight_char(T->file, pos.line, pos.column);
        return true;
    }

    return false;
}

bool tu_compile(CliState *S, TranslationUnit *T) {
    T->compile_result = compile(T->parse_result.root);

    if (T->compile_result.status) {
        CompileError *err = &T->compile_result.error;
        StringView code_str = compile_error_code_to_str(T->compile_result.status);

        elog_fmt(
            "Status: {sv}\n{cstr} ({sz}:{sz}) {s}\n",
            &code_str,
            T->file,
            err->pos.line,
            err->pos.column,
            &err->description
        );

        cli_highlight_char(T->file, err->pos.line, err->pos.column);
        return true;
    }

    return false;
}

bool tu_ast_log(CliState *S, TranslationUnit *T) {
    if (gramina_global_log_level > GRAMINA_LOG_LEVEL_VERBOSE) {
        return false;
    }

    Stream printer = mk_stream_log(GRAMINA_LOG_LEVEL_VERBOSE);
    if (!stream_is_valid(&printer)) {
        elog_fmt("Failed to create verbose logging stream\n");
        return true;
    }

    ast_print(T->parse_result.root, &printer);
    stream_free(&printer);

    return false;
}

bool tu_ast_dump(CliState *S, TranslationUnit *T) {
    if (!S->ast_dump_file) {
        return false;
    }

    Stream printer = mk_stream_open_c(S->ast_dump_file, "w");
    if (!stream_is_valid(&printer)) {
        elog_fmt("Failed to open '{cstr}': {cstr}\n", S->ast_dump_file, strerror(errno));
        return true;
    }

    ast_print(T->parse_result.root, &printer);
    stream_free(&printer);

    return false;
}

bool tu_ir_dump(CliState *S, TranslationUnit *T) {
    if (!S->out_file) {
        return false;
    }

    char *err;
    if (LLVMPrintModuleToFile(T->compile_result.module, S->out_file, &err)) {
        elog_fmt("Failed to dump IR into '{cstr}': {cstr}\n", S->out_file, err);
        LLVMDisposeErrorMessage(err);
        return true;
    }

    return false;
}

void tu_free(TranslationUnit *this) {
    stream_free(&this->fstream);

    lex_result_free(&this->lex_result);
    parse_result_free(&this->parse_result);

    if (this->compile_result.status) {
        str_free(&this->compile_result.error.description);
    }

    LLVMDisposeModule(this->compile_result.module);
}
