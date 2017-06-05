/**
* Energy reading from a Watts Up? device.
*
* @author Connor Imes
* @date 2017-01-27
*/
#ifndef _WATTSUP_DRIVER_H_
#define _WATTSUP_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#ifndef ENERGYMON_WATTSUP_VENDOR_ID
  // vendor id is "Future Technology Devices International, Ltd"
  #define ENERGYMON_WATTSUP_VENDOR_ID 0x0403
#endif

#ifndef ENERGYMON_WATTSUP_PRODUCT_ID
  // product id is "FT232 Serial (UART) IC"
  #define ENERGYMON_WATTSUP_PRODUCT_ID 0x6001
#endif

#ifndef ENERGYMON_WATTSUP_TIMEOUT_MS
  // documentation specifies response within 2 seconds
  #define ENERGYMON_WATTSUP_TIMEOUT_MS 2000
#endif

#define WU_CLEAR "#R,W,0;"
#define WU_LOG_START_EXTERNAL "#L,W,3,E,1,1;"
#define WU_LOG_STOP "#L,W,0;"
// a self-imposed max message size
#define WU_MAX_MESSAGE_SIZE 64

// opaque struct
typedef struct energymon_wattsup_ctx energymon_wattsup_ctx;

/**
 * Allocate and initialize a new context and connect to the device.
 * Not all implementations use the dev_File parameter.
 *
 * @param dev_file
 *   The device file, e.g. "/dev/ttyUSB0"
 * @param timeout_ms
 *   The read and write timeout value to use
 *
 * @return the new context
 */
energymon_wattsup_ctx* wattsup_connect(const char* dev_file, unsigned int timeout_ms);

/**
 * Disconnect from the device and free the context
 *
 * @param ctx
 *   Must not be NULL
 *
 * @return 0 on success, a negative value otherwise
 */
int wattsup_disconnect(energymon_wattsup_ctx* ctx);

/**
 * Read data from the WattsUp into the buffer.
 * Returns the number of characters read, or a negative value on failure.
 *
 * @param ctx
 *   Must not be NULL
 * @param buf
 *   Must not be NULL
 * @param buflen
 *   Must be > 0
 *
 * @return the number of characters read on success, a negative value otherwise
 */
int wattsup_read(energymon_wattsup_ctx* ctx, char* buf, size_t buflen);

/**
 * Write data to the WattsUp into the buffer.
 * Returns the number of characters written, or a negative value on failure.
 *
 * @param ctx
 *   Must not be NULL
 * @param buf
 *   Must not be NULL
 * @param buflen
 *   Must be > 0
 *
 * @return the number of characters written on success, a negative value otherwise
 */
int wattsup_write(energymon_wattsup_ctx* ctx, const char* buf, size_t buflen);

/**
 * Get a name for the implementation.
 *
 * @param buf
 *   Must not be NULL
 * @param buflen
 *   Must be > 0
 *
 * @return a pointer to the buffer on success, NULL otherwise
 */
char* wattsup_get_implementation(char* buf, size_t buflen);

#ifdef __cplusplus
}
#endif

#endif
