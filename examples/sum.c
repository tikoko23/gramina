#include <stdio.h>
#include <stdint.h>

int32_t sum(int32_t a, int32_t b);

int main(void) {
    int a = 10;
    int b = 13;
    int result = sum(a, b);

    printf("%d\n", result);

    return 0;
}
