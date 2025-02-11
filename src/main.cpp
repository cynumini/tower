#include <stdio.h>

#include "raylib.h"

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "game");

    SetTargetFPS(60);

    Image map_image = GenImagePerlinNoise(800, 800, 0, 0, 5);
    
    Texture map_texture = LoadTextureFromImage(map_image);
    UnloadImage(map_image);

    while (!WindowShouldClose()) {
	// Update
	// Draw
        BeginDrawing();
	ClearBackground(RAYWHITE);
	DrawText("It's works!", 350, 200, 20, LIGHTGRAY);
	DrawTexture(map_texture, 0, 0, WHITE);
	EndDrawing();
    }

    CloseWindow();
    return 0;
}
