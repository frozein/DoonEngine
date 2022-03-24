#ifndef DN_GLOBALS_H
#define DN_GLOBALS_H
#include <stdio.h>

#define ERROR_LOG(...) fprintf(stderr, __VA_ARGS__)
#define FLATTEN_INDEX(xp, yp, zp, size) (xp) + (size.x) * ((yp) + (zp) * (size.y))

#endif