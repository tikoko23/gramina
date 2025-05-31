#include <stdio.h>

struct Vector2 {
    float x;
    float y;
    int a; 
};

void print_float(char pre, float f) {
    printf("%c: %f\n", pre, f);
}

float vec2_length(struct Vector2 *vec);

int main(void) {
    struct Vector2 vec = {
        .x = 23,
        .y = 37,
    };

    printf("Length of (%.2f, %.2f) = %f\n", vec.x, vec.y, vec2_length(&vec));

    return 0;
}
