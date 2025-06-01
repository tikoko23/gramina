/* gen_ignore: true */

#ifndef __GRAMINA_CLI_TU_H
#define __GRAMINA_CLI_TU_H

#include "cli/state.h"

#include "compiler/compiler.h"

#include "parser/lexer.h"
#include "parser/parser.h"

typedef struct {
    const char *file;
    struct gramina_stream fstream;
    struct gramina_lex_result lex_result;
    struct gramina_parse_result parse_result;
    struct gramina_compile_result compile_result;
} TranslationUnit;

GRAMINA_DECLARE_ARRAY(TranslationUnit);

typedef bool (*CompilationStageProcessor)(CliState *S, TranslationUnit *T);

typedef struct {
    CompilationStageProcessor processor;
    const char *name;
} CompilationStage;

GRAMINA_DECLARE_ARRAY(CompilationStage);

typedef struct {
    struct gramina_array(CompilationStage) stages;
} Pipeline;

Pipeline pipeline_default(CliState *S);

void pipeline_free(Pipeline *this);

bool tu_load(CliState *S, TranslationUnit *T);
bool tu_lex(CliState *S, TranslationUnit *T);
bool tu_parse(CliState *S, TranslationUnit *T);
bool tu_compile(CliState *S, TranslationUnit *T);

bool tu_pipe(CliState *S, const Pipeline *P, TranslationUnit *T);

bool tu_ast_log(CliState *S, TranslationUnit *T);
bool tu_ast_dump(CliState *S, TranslationUnit *T);

bool tu_ir_dump(CliState *S, TranslationUnit *T);

void tu_free(TranslationUnit *this);

#endif
