/**
 * @file
 *
 * @brief Header file for useful functions regarding statistical processing of data.
 */

#ifndef UTIL_H
#define UTIL_H

//Libraries
#include <cstdint>
#include <cstddef>
#include <vector>

// Z_CI_95 is z(0.05 / 2) = z(0.025) = 1.96, for 1 - a = 95% CI
// according to https://perfeval.epfl.ch/printMe/perf.pdf, Appendix A
#define Z_CI_95 1.96

/**
 * @brief Computes the median of the elements of the given vector.
 * @returns The median of the elements of the given vector.
 */
double compute_median(std::vector<double> values);

/**
 * @brief Computes the confidence interval (CI) of the median of the first `n` elements of the vector.
 * @returns The 95% CI of the median of the first `n` elements of the vector.
 */
double compute_95_CI(std::vector<double> metrics, uint32_t n);


#endif
