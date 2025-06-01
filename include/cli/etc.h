/* gen_ignore: true */

#ifndef __GRAMINA_CLI_ETC_H
#define __GRAMINA_CLI_ETC_H

#include <llvm-c/TargetMachine.h>

#include "cli/state.h"

bool cli_handle_args(CliState *S, int argc, char **argv);

LLVMTargetMachineRef cli_get_machine();

#endif
