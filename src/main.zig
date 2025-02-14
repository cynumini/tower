const std = @import("std");
const c = @cImport({
    @cInclude("raylib.h");
});

pub fn main() !void {
    const screenWidth = 800;
    const screenHeight = 450;

    c.InitWindow(screenWidth, screenHeight, "tower");

    c.SetTargetFPS(60);

    while (!c.WindowShouldClose()) {
        // Update

        // Draw
        c.BeginDrawing();

        c.ClearBackground(c.RAYWHITE);

        c.DrawText("Congrats! You created your first window!", 190, 200, 20, c.LIGHTGRAY);

        c.EndDrawing();
    }

    c.CloseWindow();
}
