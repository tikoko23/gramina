#include <stdio.h>

struct Data {
    int a;
    long b;
    float scale;
};

// Test C compatibility
struct Data yeah(void);

int main() {
    struct Data result = yeah();

    printf(".a = %d\n.b = %ld\n.scale = %f\n", result.a, result.b, result.scale);
}
