/**
 * @file
 *
 * @brief Implementation file for the Thread struct.
 */

//Headers
#include <LatencyWorker.h>
#include <LoadWorker.h>
#include <Thread.h>

//Libraries
#include <pthread.h>
#include <stdlib.h>

Thread *newThread(void *worker) {

    Thread *t = (Thread *) malloc(sizeof(Thread));

    t->target_           = worker;
    t->created_          = false;
    t->started_          = false;
    t->completed_        = false;
    t->suspended_        = false;
    t->running_          = false;
    t->thread_exit_code_ = 0;
    t->thread_handle_    = 0;

    return t;
}

// Thread::~Thread() {
//     if (!cancel())
//         std::cerr << "WARNING: Failed to forcefully kill a thread!" << std::endl;
// }


bool create_and_start(Thread *t, void *(*runLaunchpad)(void *)) {

    //We cannot use create() and start() on Linux because pthreads API does not allow for a thread created in the suspended state. So we just do it in one shot.
    if (t->target_ != NULL) {
        pthread_attr_t attr;
        if (pthread_attr_init(&attr)) //Currently using just default flags. This may not be the optimal choice in general.
            return false;
        if (pthread_create(&(t->thread_handle_), &attr, runLaunchpad, t->target_))
            return false;
        t->created_   = true;
        t->started_   = true;
        t->suspended_ = false;
        t->running_   = true;

        return true;
    }
    return false;
}

bool join(Thread *t) {
    if (!t->created_ || !t->started_)
        return false;

    void *exit_pointer = NULL;
    int32_t failure = pthread_join(t->thread_handle_, &exit_pointer);
    if (exit_pointer)
        t->thread_exit_code_ = *((int32_t *) exit_pointer);

    if (!failure) {
        t->running_ = false;
        t->suspended_ = false;
        t->completed_ = true;
        return true;
    }
    return false;
}

// bool Thread::cancel() {
// #ifdef _WIN32
//     if (created_) {
//         if (TerminateThread(thread_handle_, -1)) { //This can be unsafe! Use with caution.
// #endif
// #ifdef __gnu_linux__
//     if (created_ && !completed_) {
//         if (pthread_cancel(thread_handle_)) {
// #endif
//             suspended_ = false;
//             running_ = false;
//             completed_ = true;
//             return true;
//         }
//         return false;
//     }
//     return true;
// }

// int32_t Thread::getExitCode() {
//     return thread_exit_code_;
// }

// bool Thread::started() {
//     return started_;
// }

// bool Thread::completed() {
//     return completed_;
// }

// bool Thread::validTarget() {
//     return (target_ != NULL);
// }

// bool Thread::created() {
//     return created_;
// }

// bool Thread::isThreadSuspended() {
//     return suspended_;
// }

// bool Thread::isThreadRunning() {
//     return running_;
// }

// Runnable* Thread::getTarget() {
//     return target_;
// }

// #ifdef _WIN32
// bool Thread::create() {
//     if (target_ != NULL) {
//         DWORD temp;
//         thread_handle_ = CreateThread(NULL, 0, &Thread::runLaunchpad, target_, CREATE_SUSPENDED, &temp);
//         if (thread_handle_ == NULL)
//             return false;
//         thread_id_ = static_cast<uint32_t>(temp);
//         created_ = true;
//         suspended_ = true;

//         return true;
//     }
//     return false;
// }
// #endif

// #ifdef _WIN32
// bool Thread::start() {
//     if (created_) {
//         if (ResumeThread(thread_handle_) == (DWORD)-1) //error condition check
//             return false;

//         started_ = true;
//         running_ = true;
//         suspended_ = false;
//         return true;
//     }
//     return false;
// }
// #endif

void *runLaunchpadLatency(void *target_runnable_object) {
    int32_t *thread_retval = (int32_t *) malloc(sizeof(int32_t));
    *thread_retval = 1;
    if (target_runnable_object != NULL) {
        LatencyWorker *target = (LatencyWorker *) target_runnable_object;
        runLatWorker(target);
        *thread_retval = 0;
        return (void *) thread_retval;
    }
    return (void *) thread_retval;
}

void *runLaunchpadLoad(void *target_runnable_object) {
    int32_t *thread_retval = (int32_t *) malloc(sizeof(int32_t));
    *thread_retval = 1;
    if (target_runnable_object != NULL) {
        LoadWorker *target = (LoadWorker *) target_runnable_object;
        runLoadWorker(target);
        *thread_retval = 0;
        return (void *) thread_retval;
    }
    return (void *) thread_retval;
}
