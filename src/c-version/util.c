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


// // A utility function to swap two elements
// void swap(int* a, int* b)
// {
//     int t = *a;
//     *a = *b;
//     *b = t;
// }

// /* This function takes last element as pivot, places
// the pivot element at its correct position in sorted
// array, and places all smaller (smaller than pivot)
// to left of pivot and all greater elements to right
// of pivot */
// int partition (int arr[], int low, int high)
// {
//     int pivot = arr[high]; // pivot
//     int i = (low - 1); // Index of smaller element and indicates the right position of pivot found so far

//     for (int j = low; j <= high - 1; j++)
//     {
//         // If current element is smaller than the pivot
//         if (arr[j] < pivot)
//         {
//             i++; // increment index of smaller element
//             swap(&arr[i], &arr[j]);
//         }
//     }
//     swap(&arr[i + 1], &arr[high]);
//     return (i + 1);
// }

// /* The main function that implements QuickSort
// arr[] --> Array to be sorted,
// low --> Starting index,
// high --> Ending index */
// void quickSort(int arr[], int low, int high)
// {
//     if (low < high)
//     {
//         /* pi is partitioning index, arr[p] is now
//         at right place */
//         int pi = partition(arr, low, high);

//         // Separately sort elements before
//         // partition and after partition
//         quickSort(arr, low, pi - 1);
//         quickSort(arr, pi + 1, high);
//     }
// }

int comparatorDoubleAsc (const void * a, const void * b) {
    // if (*(double*)a > *(double*)b)
    //     return 1;
    // else if (*(double*)a < *(double*)b)
    //     return -1;
    // else
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
