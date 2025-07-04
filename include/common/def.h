#ifndef __GRAMINA_COMMON_DEF_H
#define __GRAMINA_COMMON_DEF_H

#include <stdbool.h>


#if defined(unix) || defined(__unix__) || defined(__unix)
#  include <unistd.h>
#  include <signal.h>
#  define gramina_trigger_debugger() raise(SIGTRAP)
#  define GRAMINA_UNIX_BUILD
#endif
#ifdef _WIN32
#  include <windows.h>
#  define gramina_trigger_debugger() DebugBreak()
#endif

#ifndef GRAMINA_DEBUG_BUILD
#  undef gramina_trigger_debugger
#  define gramina_trigger_debugger() (NULL)
#endif

#ifndef gramina_assert
#  include <stdlib.h>
#  include <stdio.h>

#  define gramina_assert(cond, ...) do { \
       if (!(cond)) { \
           fprintf(stderr, "at %s:%d\n  assertion failed: %s\n", __FILE__, __LINE__, #cond); \
           if (#__VA_ARGS__[0] != '\0') { \
               fprintf(stderr, "    "__VA_ARGS__); \
           } \
            \
           gramina_trigger_debugger(); \
           exit(-1); \
       } \
   } while(0)

#  ifndef GRAMINA_DEBUG_BUILD
#    undef gramina_assert
#    define gramina_assert(...)
#  endif

#endif

#define GRAMINA_BEGIN_BLOCK_DECL() for (bool ____ffl = true; ____ffl; ____ffl = false)
#define GRAMINA_BLOCK_DECL(...) for (__VA_ARGS__; ____ffl; ____ffl = false)

#define GRAMINA_N_TH(n) (1ULL << (n))

#endif
#include "gen/common/def.h"
