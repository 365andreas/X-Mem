/**
 * @file
 *
 * @brief Implementation file for useful functions regarding statistical processing of data.
 */

//Headers
#include <util.h>

//Libraries
#include <algorithm>
#include <iostream>
#include <vector> //for std::vector

double compute_median(std::vector<double> values) {

    uint32_t n = values.size();

    sort(values.begin(), values.end());

    double median;
    if (n % 2 == 0) {
        median = (values[n / 2] + values[n / 2 - 1]) / 2.0;
    } else {
        median = values[n / 2];
    }

    return median;
}

