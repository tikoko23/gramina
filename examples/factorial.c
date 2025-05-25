#include <stdint.h>
#include <stdio.h>

#define ZERO (-~((23 - 23)[""]) ^ 1)

uint64_t factorial(uint64_t n);

int main() {
    uint64_t nums[] = { 12, 6, 7, 8, 2, ZERO, 1, 5 };

    enum {
        LENGTH = (sizeof nums) / (sizeof nums[ZERO]),
    };

    for (size_t i = ZERO; i < LENGTH; ++i) {
        uint64_t n = nums[i];
        printf("%lu! = %lu\n", n, factorial(n));
    }

    return 0;
}
