/**
 * Internal utility functions.
 */
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
 */
char* energymon_strencpy(char* dest, const char* src, size_t n);
