#define GRAMINA_NO_NAMESPACE

#include "cli/state.h"

void cli_state_free(CliState *this) {
    array_free(_GraminaArgString, &this->link_libs);
    array_free(_GraminaArgString, &this->sources);
}
