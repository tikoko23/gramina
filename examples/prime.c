#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

bool isPrime(uint64_t n);

int main() {
    uint64_t nums[] = { 23, 6, 7, 8, 2, 0, 1, 5 };

    enum {
        LENGTH = (sizeof nums) / (sizeof nums[0]),
    };

    for (size_t i = 0; i < LENGTH; ++i) {
        uint64_t n = nums[i];
        printf("%lu is%s prime\n", n, isPrime(n) ? "" : " not");
    }

    return 0;
}

