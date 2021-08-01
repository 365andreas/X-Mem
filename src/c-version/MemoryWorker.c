/**
 * @file
 *
 * @brief Implementation file for the MemoryWorker struct.
 */

//Headers
#include <MemoryWorker.h>
#include <common.h>

MemoryWorker *newMemoryWorker(void* mem_array, size_t len, int32_t cpu_affinity) {

    MemoryWorker *mem_worker = (MemoryWorker *) malloc(sizeof(MemoryWorker));

    mem_worker->runnable             = newRunnable();

    mem_worker->len_                 = len;
    mem_worker->mem_array_           = mem_array;
    mem_worker->cpu_affinity_        = cpu_affinity;
    mem_worker->bytes_per_pass_      = 0;
    mem_worker->passes_              = 0;
    mem_worker->elapsed_ticks_       = 0;
    mem_worker->elapsed_dummy_ticks_ = 0;
    mem_worker->adjusted_ticks_      = 0;
    mem_worker->warning_             = false;
    mem_worker->completed_           = false;

    return mem_worker;
}


void deleteMemoryWorker(MemoryWorker *mem_worker) {

    free(mem_worker->runnable);
    free(mem_worker);
}

// size_t MemoryWorker::getLen() {
//     size_t retval = 0;
//     if (acquireLock(-1)) {
//         retval = len_;
//         releaseLock();
//     }

//     return retval;
// }

uint32_t getBytesPerPass(MemoryWorker *mem_worker) {
    uint32_t retval = 0;
    if (acquireLock(mem_worker->runnable, -1)) {
        retval = mem_worker->bytes_per_pass_;
        releaseLock(mem_worker->runnable);
    }

    return retval;
}

uint32_t getPasses(MemoryWorker *mem_worker) {
    uint32_t retval = 0;
    if (acquireLock(mem_worker->runnable, -1)) {
        retval = mem_worker->passes_;
        releaseLock(mem_worker->runnable);
    }

    return retval;
}

tick_t getElapsedTicks(MemoryWorker *mem_worker) {
    tick_t retval = 0;
    if (acquireLock(mem_worker->runnable, -1)) {
        retval = mem_worker->elapsed_ticks_;
        releaseLock(mem_worker->runnable);
    }

    return retval;
}

tick_t getElapsedDummyTicks(MemoryWorker *mem_worker) {
    tick_t retval = 0;
    if (acquireLock(mem_worker->runnable, -1)) {
        retval = mem_worker->elapsed_dummy_ticks_;
        releaseLock(mem_worker->runnable);
    }

    return retval;
}

tick_t getAdjustedTicks(MemoryWorker *mem_worker) {
    tick_t retval = 0;
    if (acquireLock(mem_worker->runnable, -1)) {
        retval = mem_worker->adjusted_ticks_;
        releaseLock(mem_worker->runnable);
    }

    return retval;
}

bool hadWarning(MemoryWorker *mem_worker) {
    bool retval = true;
    if (acquireLock(mem_worker->runnable, -1)) {
        retval = mem_worker->warning_;
        releaseLock(mem_worker->runnable);
    }

    return retval;
}
