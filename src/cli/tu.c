#include <llvm-c/TargetMachine.h>
#include <llvm-c/Types.h>
#define GRAMINA_NO_NAMESPACE

#include <errno.h>
#include <string.h>

#include <llvm-c/Core.h>
#include <llvm-c/Error.h>
#include <llvm-c/Linker.h>

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

static char *replace_extension(const char *filename, const char *extension) {
    char *dot_loc = strrchr(filename, '.');
    size_t extension_start = dot_loc
                           ? dot_loc - filename
                           : strlen(filename);

    char *buf = malloc(extension_start + strlen(extension) + 1);

    memcpy(buf, filename, extension_start);
    memcpy(buf + extension_start, extension, strlen(extension) + 1);

    return buf;
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
        S->ir_dump_file
            ? tu_ir_dump
            : NULL,
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

    T->module = T->compile_result.module;
    T->compile_result.module = NULL;

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
    if (!S->ir_dump_file) {
        return false;
    }

    char *err;
    if (LLVMPrintModuleToFile(T->module, S->ir_dump_file, &err)) {
        elog_fmt("Failed to dump IR into '{cstr}': {cstr}\n", S->ir_dump_file, err);
        LLVMDisposeErrorMessage(err);
        return true;
    }

    return false;
}

LLVMModuleRef tu_link(CliState *S, TranslationUnit *tus, size_t n_tus) {
    LLVMModuleRef final_mod = tus->module;

    for (size_t i = 1; i < n_tus; ++i) {
        TranslationUnit *T = tus + i;

        if (LLVMLinkModules2(final_mod, T->module)) {
            return NULL;
        }

        T->module = NULL;
    }

    return final_mod;
}

bool tu_emit_objects(CliState *S, TranslationUnit *tus, size_t n_tus, ObjectFileType type) {
    if (n_tus == 0) {
        return true;
    }

    for (size_t i = 0; i < n_tus; ++i) {
        TranslationUnit *T = tus + i;

        LLVMModuleRef mod = T->module;

        char *replaced = replace_extension(T->file, ".o");
        char *err;

        if (LLVMTargetMachineEmitToFile(S->machine, mod, replaced, (LLVMCodeGenFileType)type, &err)) {
            elog_fmt("{cstr}: {cstr}\n", replaced, err);
            LLVMDisposeErrorMessage(err);
            LLVMDisposeTargetMachine(S->machine);
            free(replaced);
            return true;
        }

        free(replaced);
    }

    return false;
}

static void handler(LLVMDiagnosticInfoRef info, void *_) {
    char *msg = LLVMGetDiagInfoDescription(info);
    elog_fmt("LLVM: {cstr}\n", msg);
    LLVMDisposeMessage(msg);
}

bool tu_merge(CliState *S, TranslationUnit *out, TranslationUnit *tus, size_t n_tus) {
    if (n_tus == 0) {
        return true;
    }

    LLVMDiagnosticHandler old_handler = LLVMContextGetDiagnosticHandler(LLVMGetGlobalContext());
    void *old_context = LLVMContextGetDiagnosticContext(LLVMGetGlobalContext());

    LLVMModuleRef mod = LLVMCloneModule(tus->module);
    LLVMContextSetDiagnosticHandler(LLVMGetGlobalContext(), handler, NULL);

    for (size_t i = 1; i < n_tus; ++i) {
        TranslationUnit *T = tus + i;

        LLVMModuleRef copy = LLVMCloneModule(T->module);
        if (LLVMLinkModules2(mod, copy)) {
            elog_fmt("Failed to link '{cstr}'\n", T->file);

            LLVMContextSetDiagnosticHandler(LLVMGetGlobalContext(), old_handler, old_context);
            LLVMDisposeModule(mod);
            LLVMDisposeModule(copy);
            return true;
        }
    }

    LLVMContextSetDiagnosticHandler(LLVMGetGlobalContext(), old_handler, old_context);

    *out = (TranslationUnit) {
        .module = mod,
    };

    return false;
}

// Currently bundles all objects
bool tu_emit_binary(CliState *S, TranslationUnit *tus, size_t n_tus) {
    TranslationUnit merged;
    if (tu_merge(S, &merged, tus, n_tus)) {
        return true;
    }

    merged.file = S->out_file;

    if (tu_emit_objects(S, &merged, 1, OBJECT_FILE)) {
        tu_free(&merged);
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

    if (this->module) {
        LLVMDisposeModule(this->module);
    }

    if (this->compile_result.module) {
        LLVMDisposeModule(this->compile_result.module);
    }
}
