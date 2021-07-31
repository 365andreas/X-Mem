/**
 * @file
 *
 * @brief Header file for the Timer struct.
 */

#ifndef TIMER_H
#define TIMER_H

//Headers
#include <common.h>

//Libraries
#include <stdint.h>

typedef struct {
    tick_t ticks_per_ms_; /**< Ticks per ms for this timer. */
    float ns_per_tick_; /**< Nanoseconds per tick for this timer. */
} Timer;


/**
 * @brief Initializes timer. This may take a noticeable amount of time.
 */
Timer *newTimer();

/**
 * @brief Gets ticks per ms for this timer.
 * @returns The reported number of ticks per ms.
 */
tick_t getTicksPerMs(Timer *timer);

/**
 * @brief Gets nanoseconds per tick for this timer.
 * @returns the number of nanoseconds per tick
 */
float getNsPerTick(Timer *timer);

#endif
