const std = @import("std");
const c = @cImport({
    @cInclude("raylib.h");
    @cInclude("raymath.h");
});

const Object = struct {
    position: c.Vector2,
    size: c.Vector2,
    color: c.Color,
};

fn ySort(_: void, a: *Object, b: *Object) bool {
    return a.position.y < b.position.y;
}

pub fn main() !void {
    var screen_width: i32 = 640;
    var screen_height: i32 = 360;

    c.InitWindow(screen_width, screen_height, "tower");

    c.SetTargetFPS(60);

    var camera: c.Camera2D = .{
        .zoom = 1,
        .offset = .{ .x = 640 / 2, .y = 360 / 2 },
    };

    var player = Object{ .position = c.Vector2Zero(), .size = .{ .x = 32, .y = 64 }, .color = c.BLUE };
    var test_object = Object{ .position = c.Vector2Zero(), .size = .{ .x = 32, .y = 32 }, .color = c.GREEN };

    var objects = [_]*Object{ &player, &test_object };

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
        if (c.IsKeyDown(c.KEY_D)) player_move.x += 1;
        if (c.IsKeyDown(c.KEY_A)) player_move.x -= 1;
        if (c.IsKeyDown(c.KEY_W)) player_move.y -= 1;
        if (c.IsKeyDown(c.KEY_S)) player_move.y += 1;
        if (c.IsKeyPressed(c.KEY_F)) {
            if (c.Vector2Distance(player.position, test_object.position) <= 48) {
                std.debug.print("You use test_object\n", .{});
            }
        }

        player_move = c.Vector2Scale(c.Vector2Normalize(player_move), 4);
        player.position = c.Vector2Add(player.position, player_move);

        // camera.target = player.position;
        // camera.target = c.Vector2Add(player.position, c.Vector2Scale(player.size, 0.5));
        camera.target = player.position;
        camera.target.y = camera.target.y - player.size.y / 2;

        std.mem.sort(*Object, &objects, {}, ySort);

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

        for (objects) |object| {
            const position = c.Vector2{
                .x = object.position.x - object.size.x / 2,
                .y = object.position.y - object.size.y,
            };
            c.DrawRectangleV(
                // c.Vector2Add(object.position, c.Vector2Scale(object.size, 0.5)),
                position,
                object.size,
                object.color,
            );
        }

        c.EndMode2D();
        c.EndDrawing();
    }

    c.CloseWindow();
}
