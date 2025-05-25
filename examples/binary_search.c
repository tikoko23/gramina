#include <stdint.h>
#include <stdio.h>

uint64_t binarySearch(int64_t *array, uint64_t l, uint64_t r, int64_t x);

void check(int64_t *nums, uint64_t n, int64_t x) {
    uint64_t index = binarySearch(nums, 0, n - 1, x);

    if (index == UINT64_MAX) {
        printf("%ld doesn't exist in the array!\n", x);
    } else {
        printf("%ld is at index %lu in the array!\n", x, index);
    }
}

int main(void) {
    int64_t nums[] = { -23, -5, 0, 6, 7, 7, 37 };

    enum {
        LENGTH = (sizeof nums) / (sizeof nums[0]),
    };

    check(nums, LENGTH, 23);
    check(nums, LENGTH, -23);
    check(nums, LENGTH, 0);
    check(nums, LENGTH, 6);
    check(nums, LENGTH, 37);
    check(nums, LENGTH, 7);

    return 0;
}
