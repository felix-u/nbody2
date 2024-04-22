const builtin = @import("builtin");
const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "nbody2",
        .root_source_file = .{ .path = "src/main.zig" },
        .target = target,
        .optimize = optimize,
    });

    const raylib_build = @import("raylib_build.zig");
    const raylib = raylib_build.addRaylib(b, target, .ReleaseFast, .{});
    exe.linkLibrary(raylib);
    exe.addIncludePath(.{ .path = "raylib/src" });

    switch (builtin.os.tag) {
        inline .linux => {
            exe.linkSystemLibrary("X11");
            exe.linkSystemLibrary("xkbcommon");
        },
        inline else => {},
    }

    b.installArtifact(exe);
}
