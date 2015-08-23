/**
 * Utility functions.
 *
 * @author Connor Imes
 * @date 2015-08-23
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
char* energymon_strencpy(char* dest, const char* src, size_t n) {
  if (dest == NULL || src == NULL) {
    return NULL;
  }
  size_t i;
  for (i = 0; i < n && src[i] != '\0'; i++) {
      dest[i] = src[i];
  }
  if (i < n) {
    // null-terminate after last character
    dest[i] = '\0';
  } else if (n > 0) {
    // force last char in buffer to null
    dest[n - 1] = '\0';
  }
  return dest;
}
