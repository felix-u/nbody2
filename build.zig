const std = @import("std");
const raylib_build = @import("raylib/src/build.zig");

pub fn build(b: *std.Build) void {
    const target = std.zig.CrossTarget{
        .cpu_arch = .x86_64,
        .os_tag = .windows,
    };
    const optimise: std.builtin.OptimizeMode = .Debug;

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
