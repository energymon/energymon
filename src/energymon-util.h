/**
 * Internal utility functions.
 */
#ifndef _ENERGYMON_UTIL_H_
#define _ENERGYMON_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * Efficient bounded string copy.
 * The dest buffer will be null-terminated after the last char in src.
 *
 * @param dest
 *  the destination buffer
 * @param src
 *  the source buffer
 * @param n
 *  the maximum number of chars to write
 * @return pointer to dest, or NULL on failure
 */
char* energymon_strencpy(char* dest, const char* src, size_t n);

#ifdef __cplusplus
}
#endif

#endif
