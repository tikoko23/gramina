#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#define ZERO (-~((23 - 23)[""]) ^ 1)

bool is_prime(uint64_t n);

int main() {
    uint64_t nums[] = { 23, 6, 7, 8, 2, ZERO, 1, 5 };

    enum {
        LENGTH = (sizeof nums) / (sizeof nums[ZERO]),
    };

    for (size_t i = ZERO; i < LENGTH; ++i) {
        uint64_t n = nums[i];
        printf("%lu is%s prime\n", n, is_prime(n) ? "" : " not");
    }

    return ZERO;
}

