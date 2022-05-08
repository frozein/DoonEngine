#ifndef DN_GLOBALS_H
#define DN_GLOBALS_H

#include <stdio.h>
#include <malloc.h>

//--------------------------------------------------------------------------------------------------------------------------------//
//MEMORY FUNCTIONS USED BY LIBRARY:

#ifndef DN_ERROR_LOG
#define DN_ERROR_LOG(...) fprintf(stderr, __VA_ARGS__)
#endif

#ifndef DN_MALLOC
#define DN_MALLOC(s) malloc((s))
#endif

#ifndef DN_FREE
#define DN_FREE(p) free((p))
#endif

#ifndef DN_REALLOC
#define DN_REALLOC(p, s) realloc((p), (s))
#endif

//--------------------------------------------------------------------------------------------------------------------------------//
//UTILITY MACROS:

#define DN_FLATTEN_INDEX(p, s) (p.x) + (s.x) * ((p.y) + (p.z) * (s.y))

#endif