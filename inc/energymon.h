/**
 * API for reading energy values. Different platforms may need different
 * implementations.  For example, X86-based systems may be able to share an
 * implementation that reads from the MSR, while other systems may have to read
 * from power sensors and convert power readings to energy.
 *
 * Pointers to implementations of the typedef'd functions below are included
 * in the em_impl struct for encapsulation.
 *
 * @author Connor Imes
 * @date 2014-06-30
 */
#ifndef _ENERGYMON_H_
#define _ENERGYMON_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct em_impl em_impl;

/**
 * Open required file(s), start necessary background tasks, etc.
 * Typically allocates the state field of the em_impl struct.
 *
 * @param pointer to an em_impl
 * @return 0 on success, failure code otherwise
 */
typedef int (*em_init_func) (em_impl*);

/**
 * Get the total energy in microjoules.
 *
 * @param pointer to an em_impl
 * @return energy (in uJ)
 */
typedef unsigned long long (*em_read_total_func) (const em_impl*);

/**
 * Stop background tasks, close open file(s), free memory allocations, etc.
 * Typically frees the state field of the em_impl struct.
 *
 * @param pointer to an em_impl
 * @return 0 on success, failure code otherwise
 */
typedef int (*em_finish_func) (em_impl*);

/**
 * Get a human-readable description of the energy calculation source.
 *
 * @param pointer to a buffer
 * @return pointer to the same buffer, or NULL on failure
 */
typedef char* (*em_get_source_func) (char* buffer);

/**
 * Get the refresh interval in microseconds of the underlying sensor(s).
 *
 * @param pointer to an em_impl
 * @return the refresh interval
 */
typedef unsigned long long (*em_get_interval_func) (const em_impl*);

/**
 * A structure to encapsulate a complete implementation. The first four fields
 * are pointers to required functions. The state field is managed by the
 * implementation.
 */
struct em_impl {
  em_init_func finit;
  em_read_total_func fread;
  em_finish_func ffinish;
  em_get_source_func fsource;
  em_get_interval_func finterval;
  void* state;
};

/**
 * Get the default implementation.
 */
int em_impl_get(em_impl* impl);

#ifdef __cplusplus
}
#endif

#endif
