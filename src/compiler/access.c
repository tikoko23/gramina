#define GRAMINA_NO_NAMESPACE

#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include "compiler/access.h"
#include "compiler/errors.h"
#include "compiler/mem.h"

Value subscript(CompilerState *S, const Value *_scriptee, const Value *_scripter) {
    Value scriptee = try_load(S, _scriptee);
    Value scripter = try_load(S, _scripter);

    // TODO: operator overloading

    switch (scriptee.type.kind) {
    case GRAMINA_TYPE_ARRAY:
        if (scripter.type.kind != GRAMINA_TYPE_PRIMITIVE
         || !primitive_is_integral(scripter.type.primitive)) {
            StringView op = mk_sv_c("subscript");
            err_illegal_op(S, &scriptee.type, &scripter.type, &op);
            value_free(&scriptee);
            value_free(&scripter);
            break;
        }

        LLVMValueRef result = LLVMBuildInBoundsGEP2(
            S->llvm_builder,
            scriptee.type.llvm,
            scriptee.llvm,
            (LLVMValueRef[2]) {
                LLVMConstInt(LLVMInt32Type(), 0, false),
                scripter.llvm,
            }, 2, ""
        );

        LLVMValueRef loaded = kind_is_aggregate(scriptee.type.element_type->kind)
                            ? result
                            : LLVMBuildLoad2(S->llvm_builder, scriptee.type.element_type->llvm, result, "");

        Value ret = {
            .type = type_dup(scriptee.type.element_type),
            .llvm = loaded,
            .class = GRAMINA_CLASS_LVALUE,
            .lvalue_ptr = result,
        };

        value_free(&scriptee);
        value_free(&scripter);

        return ret;
    case GRAMINA_TYPE_POINTER:
        if (scriptee.type.pointer_type->kind == GRAMINA_TYPE_VOID) {
            StringView op = mk_sv_c("subscript");
            err_illegal_op(S, &scriptee.type, &scripter.type, &op);
            value_free(&scriptee);
            value_free(&scripter);
            break;
        }

        break;
    default: {
        StringView op = mk_sv_c("subscript");
        err_illegal_op(S, &scriptee.type, &scripter.type, &op);
        value_free(&scriptee);
        value_free(&scripter);
        break;
    }
    }

    value_free(&scriptee);
    value_free(&scripter);

    return invalid_value();
}
