#ifndef DEFS_H
#define DEFS_H

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#define CEIL_DIV(a, b) (((a) + (b) - 1) / (b))

#endif
