/**
 * @file
 *
 * @brief Header file for the Runnable struct.
 */

#ifndef RUNNABLE_H
#define RUNNABLE_H

//Libraries
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

/**
 * @brief A base class for any object that implements a thread-safe run() function for use by Thread objects.
 */
typedef struct {
    /** A handle to the OS native Windows mutex, i.e., the locking mechanism. Outside the constructor, this should only be accessed via _acquireLock() and _releaseLock(). */
    /** A handle to the OS pthreads mutex, i.e., the locking mechanism. Outside the constructor, this should only be accessed via _acquireLock() and _releaseLock(). */
    pthread_mutex_t mutex_;
} Runnable;

/**
 * @brief Constructor.
 */
Runnable *newRunnable();

// /**
//  * @brief Destructor.
//  */
// ~Runnable();

/**
 * @brief Acquires the object lock to access all object state in thread-safe manner.
 * @param timeout timeout in milliseconds to acquire the lock. If 0, does not wait at all. If negative, waits indefinitely.
 * @returns true on success. If not successful, the lock was not acquired, possibly due to a timeout, or the lock might already be held.
 */
bool acquireLock(Runnable *runnable, int32_t timeout);

/**
 * @brief Releases the object lock to access all object state in thread-safe manner.
 * @returns true on success. If not successful, the lock is either still held or the call was illegal (e.g., releasing a lock that was never acquired).
 */
bool releaseLock(Runnable *runnable);



#endif
