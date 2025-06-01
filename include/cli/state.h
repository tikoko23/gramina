/* gen_ignore: true */

#ifndef __GRAMINA_CLI_STATE_H
#define __GRAMINA_CLI_STATE_H

#include <llvm-c/TargetMachine.h>

#include "common/arg.h"

typedef struct {
    struct gramina_array(_GraminaArgString) link_libs;
    struct gramina_array(_GraminaArgString) sources;
    const char *self_path;
    const char *out_file;
    const char *ast_dump_file;
    const char *ir_dump_file;

    enum {
        COMPILATION_STAGE_ASM,
        COMPILATION_STAGE_OBJ,
        COMPILATION_STAGE_BIN,
    } max_stage;

    LLVMTargetMachineRef machine;
} CliState;

void cli_state_free(CliState *this);

#endif
