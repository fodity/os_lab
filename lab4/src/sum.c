#include "sum.h"
long long CalculateSum(int *array, int begin, int end) {
    long long sum = 0;
    for (int i = begin; i < end; i++) {
        sum += array[i];
    }
    return sum;
}