#define GRAMINA_NO_NAMESPACE

#include <llvm-c/Transforms/PassBuilder.h>

#include "compiler/cstate.h"
#include "compiler/struct.h"
#include "compiler/function.h"

#include "parser/ast.h"

GRAMINA_IMPLEMENT_ARRAY(_GraminaReflection);

static int llvm_init(CompilerState *S) {
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();

    const char *module_name = "base"; // WIP
    S->llvm_module = LLVMModuleCreateWithName(module_name);
    S->llvm_builder = LLVMCreateBuilder();

    char *triple = LLVMGetDefaultTargetTriple(); // WIP
    LLVMSetTarget(S->llvm_module, triple);
    LLVMDisposeMessage(triple);

    return 0;
}

static int llvm_deinit(CompilerState *S) {
    LLVMDisposeBuilder(S->llvm_builder);

    return 0;
}

CompileResult gramina_compile(AstNode *root) {
    CompilerState S = {
        .has_error = false,
        .scopes = mk_array(GraminaScope),
        .reflection = mk_array(_GraminaReflection),
    };

    S.status = llvm_init(&S);
    if (S.status) {
        return (CompileResult) {
            .status = GRAMINA_COMPILE_ERR_LLVM,
            .error = {
                .description = mk_str_c("LLVM encountered an error during initialisation"),
                .pos = { 0, 0, 0 },
            },
        };
    }

    array_append(GraminaScope, &S.scopes, mk_scope());

    S.status = 0;
    AstNode *cur = root;
    do {
        switch (cur->left->type) {
        case GRAMINA_AST_FUNCTION_DEF:
        case GRAMINA_AST_FUNCTION_DECLARATION:
            function_def(&S, cur->left);
            break;
        case GRAMINA_AST_STRUCT_DEF:
            struct_def(&S, cur->left);
            break;
        default:
            break;
        }
    } while ((cur = cur->right) && !S.status);

    if (S.status) {
        array_foreach_ref(GraminaScope, _, s, S.scopes) {
            scope_free(s);
        }

        array_free(GraminaScope, &S.scopes);
        array_free(_GraminaReflection, &S.reflection);

        llvm_deinit(&S);
        LLVMDisposeModule(S.llvm_module);

        return (CompileResult) {
            .status = S.status,
            .error = S.error,
        };
    }

    gramina_assert(S.scopes.length == 1, "got: %zu", S.scopes.length);
    scope_free(array_last(GraminaScope, &S.scopes));
    array_free(GraminaScope, &S.scopes);
    array_free(_GraminaReflection, &S.reflection);

    S.status = llvm_deinit(&S);
    if (S.status) {
        LLVMDisposeModule(S.llvm_module);
        return (CompileResult) {
            .status = GRAMINA_COMPILE_ERR_LLVM,
            .error = {
                .description = mk_str_c("LLVM encountered an error during de-initialisation"),
                .pos = { 0, 0, 0 },
            },
        };
    }

    return (CompileResult) {
        .module = S.llvm_module,
        .status = GRAMINA_COMPILE_ERR_NONE,
    };
}


// s/GRAMINA_COMPILE_ERR_\(\w\+\).*/case GRAMINA_COMPILE_ERR_\1:\r        return mk_sv_c("\1");
StringView gramina_compile_error_code_to_str(CompileErrorCode e) {
    switch (e) {
    case GRAMINA_COMPILE_ERR_NONE:
        return mk_sv_c("NONE");
    case GRAMINA_COMPILE_ERR_LLVM:
        return mk_sv_c("LLVM");
    case GRAMINA_COMPILE_ERR_DUPLICATE_IDENTIFIER:
        return mk_sv_c("DUPLICATE_IDENTIFIER");
    case GRAMINA_COMPILE_ERR_UNDECLARED_IDENTIFIER:
        return mk_sv_c("UNDECLARED_IDENTIFIER");
    case GRAMINA_COMPILE_ERR_REDECLARATION:
        return mk_sv_c("REDECLARATION");
    case GRAMINA_COMPILE_ERR_INCOMPATIBLE_TYPE:
        return mk_sv_c("INCOMPATIBLE_TYPE");
    case GRAMINA_COMPILE_ERR_ILLEGAL_NODE:
        return mk_sv_c("ILLEGAL_NODE");
    case GRAMINA_COMPILE_ERR_MISSING_RETURN:
        return mk_sv_c("MISSING_RETURN");
    case GRAMINA_COMPILE_ERR_INCOMPATIBLE_VALUE_CLASS:
        return mk_sv_c("INCOMPATIBLE_VALUE_CLASS");
    case GRAMINA_COMPILE_ERR_MISSING_ATTRIB_ARG:
        return mk_sv_c("MISSING_ATTRIB_ARG");
    default:
        return mk_sv_c("UNKNOWN");
    }
}
