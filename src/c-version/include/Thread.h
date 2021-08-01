/**
 * @file
 *
 * @brief Header file for the Thread struct.
 */

#ifndef THREAD_H
#define THREAD_H

//Headers
#include <LatencyWorker.h>

//Libraries
#include <stdint.h>
#include <pthread.h>

/**
 * @brief a nice wrapped thread interface independent of particular OS API
 */
typedef struct {

    LatencyWorker *target_; /**< The object connecting a run() method which operates in a thread-safe manner. */
    bool created_; /**< If true, the OS thread has been created at some point, but does not indicate its current state. */
    bool started_; /**< If true, the OS thread has been started at some point, but does not indicate its current state. */
    bool completed_; /**< If true, the OS thread completed. */
    bool suspended_; /**< If true, the OS thread is suspended. */
    bool running_; /**< If true, the OS thread is running. */
    int32_t thread_exit_code_; /**< Contains the thread's exit status code. If it has not yet exited, this should be 0 (normal condition). */
    /** A handle to the OS thread. */
    pthread_t thread_handle_;
} Thread;

/**
 * Constructor. Does not actually create the real thread or run it.
 * @param target The target object to do some work with in a new thread.
 */
Thread *newThread(LatencyWorker *target);

// /**
//  * Destructor. Immediately cancels the thread if it exists. This can be unsafe!
//  */
// ~Thread();

/**
 * Creates and starts the thread immediately if the target Runnable is valid. This invokes the run() method in the Runnable target that was passed in the constructor.
 * @returns true if the thread was successfully created and started.
 */
bool create_and_start(Thread *t);

/**
 * Blocks the calling thread until the worker thread managed by this object terminates. For simplicity, this does not support a timeout due to pthreads incompatibility with the Windows threading API.
 * If the worker thread has already terminated, returns immediately.
 * If the worker has not yet started, returns immediately.
 * @returns true if the worker thread terminated successfully, false otherwise.
 */
bool join(Thread *t);

// /**
//  * Cancels the worker thread immediately. This should only be done in emergencies, as it is effectively killed and undefined behavior might occur.
//  * @returns true if the worker thread was successfully killed.
//  */
// bool cancel();

// /**
//  * @returns the exit code of the worker thread if it completed. If it did not complete or has not started, returns 0.
//  */
// int32_t getExitCode();

// /**
//  * @returns true if the thread has been started, regardless if has completed or not.
//  */
// bool started();

// /**
//  * @returns true if the thread completed, regardless of the manner in which it terminated. Returns false if it has not been started.
//  */
// bool completed();

// /**
//  * @returns true if the Runnable target is valid.
//  */
// bool validTarget();

// /**
//  * @returns true if the thread has been created successfully.
//  */
// bool created();

// /**
//  * @returns true if the thread is suspended. Returns false if the thread has not been created.
//  */
// bool isThreadSuspended();

// /**
//  * @returns true if the thread is running. Returns false if the thread has not been created.
//  */
// bool isThreadRunning();

// /**
//  * @returns a pointer to the target Runnable object
//  */
// Runnable* getTarget();

/**
 * Invokes the runLatWorker() method on the target LatencyWorker object.
 * @param target_lat_worker_object pointer to the target LatencyWorker object. This needs to be a generic pointer to keep APIs happy.
 */
void* runLaunchpad(void* target_lat_worker_object);

#endif
