/**
 * @file
 *
 * @brief Implementation file for the Timer struct.
 */

//Headers
#include <Timer.h>
#include <common.h>

//Libraries
#include <time.h>

Timer *newTimer() {

    Timer *timer = malloc(sizeof(Timer));

    timer->ticks_per_ms_ = 0;
    timer->ns_per_tick_ = 0;

    tick_t start_tick, stop_tick;
    start_tick = start_timer();
    struct timespec duration, remainder;
    duration.tv_sec = BENCHMARK_DURATION_MS / 1000;
    duration.tv_nsec = (BENCHMARK_DURATION_MS % 1000) * 1e6;
    nanosleep(&duration, &remainder);
    stop_tick = stop_timer();

    timer->ticks_per_ms_ = (tick_t) ((stop_tick - start_tick) / BENCHMARK_DURATION_MS);
    timer->ns_per_tick_ = 1 / ((float) (timer->ticks_per_ms_)) * (float) (1e6);

    return timer;
}

tick_t getTicksPerMs(Timer *timer) {
    return timer->ticks_per_ms_;
}

float getNsPerTick(Timer *timer) {
    return timer->ns_per_tick_;
}
