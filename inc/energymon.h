/**
 * A common API for reading energy data.
 *
 * Pointers to implementations of the typedef'd functions below are included
 * in the energymon struct for encapsulation.
 * Energy reader implementations usually expose a getter function to populate
 * an energymon struct with their own function pointers.
 *
 * The typical use case is:
 *  1) Get the energymon using an implementation's getter function.
 *  2) Initialize the energymon (finit).
 *  3) Use energymon until no longer needed (fread, etc.).
 *  4) Destroy the energymon (ffinish).
 *
 * The behavior of operating on an uninitialized energymon is undefined.
 * Implementations make a best effort to verify initialization and return 
 * proper error codes (for example, by checking if the "state" field is NULL).
 * Errno should be set (or preserved) on failures.
 * Error messages may be printed preemptively if too much context would be lost
 * otherwise.
 *
 * Functions that take energymon pointers as a parameter usually fail if the
 * pointer is NULL or if the energymon is not initialized.
 * If the implementation does not require the pointer or the energymon to be
 * initialized, it may choose whether to fail or complete its task.
 *
 * @author Connor Imes
 * @date 2014-06-30
 */
#ifndef _ENERGYMON_H_
#define _ENERGYMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>

typedef struct energymon energymon;

/**
 * Initialize the energymon.
 * Opens required file(s), starts necessary background tasks, etc.
 * Typically allocates the state field of the energymon struct.
 *
 * @param pointer to an energymon
 * @return 0 on success, failure code otherwise
 */
typedef int (*energymon_init) (energymon*);

/**
 * Get the total energy in microjoules.
 *
 * @param pointer to an energymon
 * @return energy (in uJ), or 0 on failure (errno MUST be set)
 */
typedef uint64_t (*energymon_read_total) (const energymon*);

/**
 * Destroy the energymon.
 * Stop background tasks, close open file(s), free memory allocations, etc.
 * Typically frees the state field of the energymon struct.
 *
 * @param pointer to an energymon
 * @return 0 on success, failure code otherwise
 */
typedef int (*energymon_finish) (energymon*);

/**
 * Get a human-readable description of the energy monitoring source.
 * Implementations should ensure that the buffer is null-terminated.
 *
 * @param pointer to a buffer
 * @param the maximum number of bytes to write
 * @return pointer to the same buffer, or NULL on failure
 */
typedef char* (*energymon_get_source) (char*, size_t);

/**
 * Get the refresh interval in microseconds of the underlying sensor(s).
 * This value should be greater than 0 except in error cases.
 * If there is no minimum interval, return 1.
 *
 * @param pointer to an energymon
 * @return the refresh interval (in usec), or 0 on error
 */
typedef uint64_t (*energymon_get_interval) (const energymon*);

/**
 * Get the best possible possible read precision in microjoules (round down).
 * For implementations that read from power sensors, this is a function of the
 * precision of the power readings and the refresh interval.
 * If 0 < precision <= 1, return 1.
 * If the precision is unknown, return 0 but don't set errno.
 *
 * @param pointer to an energymon
 * @return the precision, or 0 on error (set errno) or if unknown (no errno)
 */
typedef uint64_t (*energymon_get_precision) (const energymon*);

/**
 * Get whether the implementation requires exclusive access to the underlying
 * sensor(s).
 * In such cases it may be beneficial to run in a separate process and expose
 * energy data over shared memory (or other means) so multiple applications can
 * use the data source simultaneously.
 *
 * @return 0 if exclusive access is NOT required, something else otherwise
 */
typedef int (*energymon_is_exclusive) (void);

/**
 * A structure to encapsulate a complete implementation.
 * All function pointers are required.
 * The state field is managed by the implementation.
 */
struct energymon {
  energymon_init finit;
  energymon_read_total fread;
  energymon_finish ffinish;
  energymon_get_source fsource;
  energymon_get_interval finterval;
  energymon_get_precision fprecision;
  energymon_is_exclusive fexclusive;
  void* state;
};

#ifdef __cplusplus
}
#endif

#endif
