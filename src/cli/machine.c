#define GRAMINA_NO_NAMESPACE

#include <llvm-c/Core.h>
#include <llvm-c/Error.h>
#include <llvm-c/TargetMachine.h>

#include "cli/etc.h"

#include "common/log.h"

LLVMTargetMachineRef cli_get_machine() {
    char *err;
    char *triple = LLVMGetDefaultTargetTriple();
    LLVMTargetRef target;
    if (LLVMGetTargetFromTriple(triple, &target, &err)) {
        elog_fmt("LLVM target triple: {cstr}\n", err);
        LLVMDisposeErrorMessage(err);
        LLVMDisposeMessage(triple);
        return NULL;
    }

    ilog_fmt("Determined target triple: {cstr}\n", triple);

    LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
        target,
        triple,
        "generic",
        "",
        LLVMCodeGenLevelDefault,
        LLVMRelocDefault,
        LLVMCodeModelDefault
    );

    LLVMDisposeMessage(triple);

    return machine;
}
