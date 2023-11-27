const std = @import("std");

pub fn build(b: *std.Build) void {
    const cc_flags_shared = [_][]const u8{
        "-std=c99",
        "-Wall",
        "-Wextra",
        "-pedantic",
        "-Werror",
        "-Wshadow",
        "-fno-strict-aliasing",
        "-Wstrict-overflow",
    };

    const cc_flags_debug = [_][]const u8{
        "-gcodeview",
        "-fsanitize=undefined",
    };

    const cc_flags = cc_flags_shared ++ cc_flags_debug;

    const exe = b.addExecutable(.{
        .name = "nbody2",
        .target = .{
            .cpu_arch = .x86_64,
            .os_tag = .windows,
        },
        .optimize = .Debug,
        .link_libc = true,
    });

    exe.addCSourceFile(.{
        .file = .{ .path = "src/main.c" },
        .flags = &cc_flags,
    });

    exe.linkSystemLibrary("gdi32");
    exe.linkSystemLibrary("Xinput1_4");
    exe.linkSystemLibrary("Dsound");

    b.installArtifact(exe);
}
