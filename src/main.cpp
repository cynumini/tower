#include "raylib.h"

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "game");

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
	// Update
	// Draw
        BeginDrawing();
	ClearBackground(RAYWHITE);
	DrawText("It's works!", 350, 200, 20, LIGHTGRAY);
	EndDrawing();
    }

    CloseWindow();
    return 0;
}
