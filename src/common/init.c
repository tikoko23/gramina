#include "common/init.h"
#include "common/log.h"

void gramina_init() {}

void gramina_cleanup() {
    __gramina_log_cleanup();
}
