#include <stdio.h>

typedef struct {
    float x;
    float y;
} Vector2;

typedef struct {
    Vector2 position;
    Vector2 velocity;
} Player;

void move(Player *self, float dt);

int main(void) {
    Player player = {
        .position = { 1,  2 },
        .velocity = { 3, -1 },
    };

    move(&player, 1.5f);

    printf("player.position = { %f, %f }\n", player.position.x, player.position.y);

    return 0;
}
