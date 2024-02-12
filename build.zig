const std = @import("std");
const raylib_build = @import("raylib/src/build.zig");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimise = b.standardOptimizeOption(.{});

    const raylib = raylib_build.addRaylib(b, target, optimise, .{});

    const exe = b.addExecutable(.{
        .name = "nbody2",
        .root_source_file = .{ .path = "src/main.zig" },
        .target = target,
        .optimize = optimise,
    });
    exe.linkLibrary(raylib);
    exe.addIncludePath(.{ .path = "raylib/src" });

    b.installArtifact(exe);
}
