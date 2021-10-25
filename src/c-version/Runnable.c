/**
 * @file
 *
 * @brief Implementation file for the Runnable struct.
 */

//Headers
#include <Runnable.h>

//Libraries
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

Runnable *newRunnable() {

    Runnable *runnable = (Runnable *) malloc(sizeof(Runnable));

    runnable->mutex_ = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

    return runnable;
}

bool acquireLock(Runnable *runnable, int32_t timeout) {
    int32_t reason;

    if (timeout < 0) {
        reason = pthread_mutex_lock(&(runnable->mutex_));
        if (! reason) //success
            return true;
        else {
            fprintf(stderr, "WARNING: Failed to acquire lock in a Runnable object! Error code %d\n", reason);
            return false;
        }
    } else {
        struct timespec t;
        t.tv_sec = (time_t) (timeout/1000);
        t.tv_nsec = (time_t) ((timeout % 1000) * 1e6);
        reason = pthread_mutex_timedlock(&(runnable->mutex_), &t);
        if (! reason) //success
            return true;
        else if (reason != ETIMEDOUT) {
            fprintf(stderr, "WARNING: Failed to acquire lock in a Runnable object! Error code %d\n", reason);
            return false;
        }
    }

    return false;
}

bool releaseLock(Runnable *runnable) {
    int32_t retval = pthread_mutex_unlock(&(runnable->mutex_));
    if (! retval)
        return true;
    fprintf(stderr, "WARNING: Failed to release lock in a Runnable object! Error code %d\n", retval);

    return false;
}
