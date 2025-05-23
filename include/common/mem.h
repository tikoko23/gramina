#ifndef __GRAMINA_COMMON_MEM_H
#define __GRAMINA_COMMON_MEM_H

#if !defined(gramina_malloc) && !defined(gramina_realloc) && !defined(gramina_free)
#define GRAMINA_CUSTOM_ALLOC 0

#include <stdlib.h>

#define gramina_malloc malloc
#define gramina_realloc realloc
#define gramina_free free

#else
#define GRAMINA_CUSTOM_ALLOC 1

#if !(defined(gramina_malloc) && defined(gramina_realloc) && defined(gramina_free))
#  error "'gramina_malloc', 'gramina_realloc' and 'gramina_free' must all be defined if one is"
#endif
#endif

#endif
#include "gen/common/mem.h"