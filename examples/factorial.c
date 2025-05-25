#include <stdint.h>
#include <stdio.h>

uint64_t factorial(uint64_t n);

int main() {
    uint64_t nums[] = { 12, 6, 7, 8, 2, 0, 1, 5 };

    enum {
        LENGTH = (sizeof nums) / (sizeof nums[0]),
    };

    for (size_t i = 0; i < LENGTH; ++i) {
        uint64_t n = nums[i];
        printf("%lu! = %lu\n", n, factorial(n));
    }

    return 0;
}
