/**
 * @file
 *
 * @brief Header file for useful functions regarding statistical processing of data.
 */

#ifndef UTIL_H
#define UTIL_H

//Libraries
#include <stdint.h>
// #include <cstddef>

// Z_CI_95 is z(0.05 / 2) = z(0.025) = 1.96, for 1 - a = 95% CI
// according to https://perfeval.epfl.ch/printMe/perf.pdf, Appendix A
#define Z_CI_95 1.96

/**
 * @brief Useful for qsort()-ing doubles.
 */
int comparatorDoubleAsc (const void * a, const void * b);

/**
 * @brief Computes the median of the elements of the given vector.
 * @returns The median of the elements of the given vector.
 */
double compute_median(double *values, uint32_t n);

/**
 * @brief Computes the square root of the given number. It is essential since sqrt() insert fucomi instruction
 * which is not supported by k1om.
 * @returns The square root of the given number.
 */
double pseudosqrt(uint32_t n);

#endif
