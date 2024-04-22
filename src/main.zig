const Game = @import("Game.zig");
const std = @import("std");

pub fn main() !void {
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();

    var game = Game.init(.{
        .allocator = arena.allocator(),
        .name = "nbody2",
        .width = 2560,
        .height = 1440,
    });
    defer game.deinit();

    while (!game.shouldQuit()) {
        game.frameBegin();
        defer game.frameEnd();
        try game.updateAndRender();
    }
}
