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
#ifndef _EM_GENERIC_H_
#define _EM_GENERIC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "em-generic.h"

typedef struct em_impl em_impl;

/**
 * Open required file(s), start necessary background tasks, etc.
 * Typically allocates the state field of the em_impl struct.
 */
typedef int (*em_init_func) (em_impl*);

/**
 * Get the total energy.
 */
typedef double (*em_read_total_func) (em_impl*);

/**
 * Stop background tasks, close open file(s), free memory allocations, etc.
 * Typically frees the state field of the em_impl struct.
 */
typedef int (*em_finish_func) (em_impl*);

/**
 * Get a human-readable description of the energy calculation source.
 */
typedef char* (*em_get_source_func) (char* buffer);

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
  void* state;
};

/*
 * Compile with EM_GENERIC to allow code to access a default energy monitor.
 * This allows implementations to be swapped without making code changes, only
 * re-linking.
 */
#ifdef EM_GENERIC
/**
 * Get functions specific to this implementation.
 */
int em_impl_get(em_impl* impl);
#endif

#ifdef __cplusplus
}
#endif

#endif
