#define GRAMINA_NO_NAMESPACE

#include <stdio.h>
#include <string.h>

#include "cli/help.h"

#include "common/log.h"

void cli_show_general_help() {
    const char *help =
        "Usage: gramina [options] files...\n"
        "Options:\n"
        "\t--help [topic?]                  Show this message or help on a specific topic (`--help help` for a list of topics)\n"
        "\t-v, --verbose                    Enable verbose logging\n"
        "\t--log-level [level]              Set the log level (`--help log-level` for more information)\n"
        "\t                                 Level may be 'all', 'info', 'warn', 'error', 'silent' or 'none'\n"
        "\t--ir-dump [file]                 Print the created LLVM IR into the given file\n"
        "\t--ast-dump [file]                Print the created AST into the given file\n"
        "\t-l [libname]                     Declare a library to be linked (WIP)\n"
        "\t-s, --stage [stage]              Set the stage at which the compilation will stop\n"
        "\t--linker [prog]                  Use the given program as the linker\n"
        "\t--keep-temps                     Do not remove temporary object files created after use\n"
        "";

    printf("%s", help);
}

void cli_show_specific_help(const char *topic) {
    if (false) {
    } else if (strcmp(topic, "help") == 0) {
        const char *help_help =
            "List of available help topics:\n"
            "\thelp\n"
            "\tlog-level\n"
            "\tstage\n"
            "";

        printf("%s", help_help);

    } else if (strcmp(topic, "log-level") == 0) {
        const char *log_level_help =
            "List of available levels:\n"
            "\tall (equivalent to '-v' and '--verbose')\n"
            "\tinfo\n"
            "\twarn (default)\n"
            "\terror\n"
            "\tsilent (disables all logs except ones which shouldn't be ignored)\n"
            "\tnone (disables every level of logging)\n"
            "";

        printf("%s", log_level_help);

    } else if (strcmp(topic, "stage") == 0) {
        const char *stage_help =
            "List of available stages:\n"
            "\ta, asm, assembly         (assembly file)\n"
            "\to, obj, object           (object file)\n"
            "\tb, bin, binary           (library or executable)\n"
            "";

        printf("%s", stage_help);

    } else {
        elog_fmt("No specific help for topic '{cstr}'\n", topic);
    }
}
