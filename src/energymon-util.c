/**
 * Utility functions.
 *
 * @author Connor Imes
 * @date 2015-08-23
 */

#include <errno.h>
#include <stddef.h>

/**
 * Efficient bounded string copy.
 * The dest buffer will be null-terminated after the last char in src.
 * Only fails if dest or src are null - errno will be set to EINVAL.
 *
 * @param dest
 *  the destination buffer
 * @param src
 *  the source buffer
 * @param n
 *  the maximum number of chars to write
 * @return pointer to dest, or NULL on failure
 */
char* energymon_strencpy(char* dest, const char* src, size_t n) {
  if (dest == NULL || src == NULL) {
    errno = EINVAL;
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
