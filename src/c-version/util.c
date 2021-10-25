/**
 * @file
 *
 * @brief Implementation file for useful functions regarding statistical processing of data.
 */

//Headers
#include <util.h>

//Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int comparatorDoubleAsc (const void * a, const void * b) {
    if (*(double*)a > *(double*)b)
        return 1;
    else if (*(double*)a < *(double*)b)
        return -1;
    else
        return 0;
}

double compute_median(double *values, uint32_t n) {

    double *sortedMetrics = (double *) malloc(n * sizeof(double));
    memcpy(sortedMetrics, values, n * sizeof(double));

    qsort(sortedMetrics, n, sizeof(double), comparatorDoubleAsc);

    double median;
    if (n % 2 == 0) {
        median = (sortedMetrics[n / 2] + sortedMetrics[n / 2 - 1]) / 2.0;
    } else {
        median = sortedMetrics[n / 2];
    }

    return median;
}

double pseudosqrt(uint32_t n) {

    if (n > 20) {
        fprintf(stderr, "ERROR: Value given to pseudosqrt() is greater than 20.\n");
        exit(EXIT_FAILURE);
    }
    switch (n) {
        case 0:
            return 0.0;
            break;
        case 1:
            return 1.0;
            break;
        case 2:
            return 1.4142135623730951;
            break;
        case 3:
            return 1.7320508075688772;
            break;
        case 4:
            return 2.0;
            break;
        case 5:
            return 2.23606797749979;
            break;
        case 6:
            return 2.449489742783178;
            break;
        case 7:
            return 2.6457513110645907;
            break;
        case 8:
            return 2.8284271247461903;
            break;
        case 9:
            return 3.0;
            break;
        case 10:
            return 3.1622776601683795;
            break;
        case 11:
            return 3.3166247903554;
            break;
        case 12:
            return 3.4641016151377544;
            break;
        case 13:
            return 3.605551275463989;
            break;
        case 14:
            return 3.7416573867739413;
            break;
        case 15:
            return 3.872983346207417;
            break;
        case 16:
            return 4.0;
            break;
        case 17:
            return 4.123105625617661;
            break;
        case 18:
            return 4.242640687119285;
            break;
        case 19:
            return 4.358898943540674;
            break;
        case 20:
            return 4.47213595499958;
            break;
        default:
            exit(EXIT_FAILURE);
            break;
        }
}
