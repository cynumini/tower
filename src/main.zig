const std = @import("std");
const c = @cImport({
    @cInclude("raylib.h");
    @cInclude("raymath.h");
});

pub fn main() !void {
    var screen_width: i32 = 640;
    var screen_height: i32 = 360;
    var player_position = c.Vector2Zero();

    c.InitWindow(screen_width, screen_height, "tower");

    c.SetTargetFPS(60);

    var camera: c.Camera2D = .{
        .zoom = 1,
        .offset = .{ .x = 640 / 2, .y = 360 / 2 },
    };

    while (!c.WindowShouldClose()) {
        // Update
        if (c.IsWindowResized()) {
            screen_width = c.GetScreenWidth();
            screen_height = c.GetScreenHeight();
            camera = .{
                .zoom = @as(f32, @floatFromInt(screen_height)) / 360,
                .offset = .{
                    .x = @floatFromInt(@divExact(screen_width, 2)),
                    .y = @floatFromInt(@divExact(screen_height, 2)),
                },
            };
        }
        var player_move = c.Vector2Zero();
        if (c.IsKeyDown(c.KEY_RIGHT)) player_move.x += 1;
        if (c.IsKeyDown(c.KEY_LEFT)) player_move.x -= 1;
        if (c.IsKeyDown(c.KEY_UP)) player_move.y -= 1;
        if (c.IsKeyDown(c.KEY_DOWN)) player_move.y += 1;

        player_move = c.Vector2Scale(c.Vector2Normalize(player_move), 4);
        player_move.y /= 2;
        player_position = c.Vector2Add(player_position, player_move);

        camera.target = player_position;

        // Draw
        c.BeginDrawing();

        c.ClearBackground(c.RAYWHITE);

        c.BeginMode2D(camera);

        c.DrawText(
            "Congrats! You created your first window!",
            190,
            200,
            20,
            c.LIGHTGRAY,
        );
        c.DrawEllipse(
            0,
            0,
            @divExact(640, 2),
            @divExact(640, 4),
            c.RED,
        );

        c.DrawRectangleV(
            c.Vector2{ .x = player_position.x - 16, .y = player_position.y - 16 },
            c.Vector2{ .x = 32, .y = 32 },
            c.BLUE,
        );

        c.EndMode2D();
        c.EndDrawing();
    }

    c.CloseWindow();
}
