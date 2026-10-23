#!/usr/bin/env ruby

require "fileutils"

ROOT_DIR = File.expand_path("..", __dir__)
TEMPLATE_DIR = File.join(ROOT_DIR, "userspace/rootfs")
ROOTFS_DIR = File.join(ROOT_DIR, "build/rootfs")
SOURCES_DIR = File.join(ROOT_DIR, "userspace/programs")
BUILD_DIR = File.join(ROOT_DIR, "build")

TEMPLATE_DIRECTORIES = [
    "System/Library/Frameworks",
    "System/Library/Extensions",
    "System/Library/CoreServices",
    "System/Library/LaunchAgents",
    "System/Library/LaunchDaemons",
    "Library",
    "Library/Application Support",
    "Library/Preferences",
    "Library/Preferences/SystemConfiguration",
    "Library/LaunchAgents",
    "Library/LaunchDaemons",
    "Applications",
    "Volumes",
    "usr/bin",
    "usr/lib",
    "usr/libexec",
    "private/etc",
    "private/var",
    "private/tmp"
].freeze

def build_macho(source_file, output_file)
    command = [
        "clang",
        "--target=x86_64-apple-darwin10",
        "-ffreestanding",
        "-mno-red-zone",
        "-nostdlib",
        "-fuse-ld=lld",
        "-Wl,-e,_start",
        "-Wl,-no_pie",
        source_file,
        "-o",
        output_file
    ]
    return if system(*command)

    status = $?
    raise "Failed to compile #{source_file} (exit status #{status&.exitstatus || "unknown"})"
end

def make_initrd
    FileUtils.mkdir_p([BUILD_DIR, TEMPLATE_DIR])
    FileUtils.rm_rf(ROOTFS_DIR)
    FileUtils.mkdir_p(ROOTFS_DIR)
    Dir.children(TEMPLATE_DIR).each do |entry|
        FileUtils.cp_r(File.join(TEMPLATE_DIR, entry), ROOTFS_DIR)
    end
    FileUtils.mkdir_p(TEMPLATE_DIRECTORIES.map { |path| File.join(ROOTFS_DIR, path) })

    # Find all *.c source files in the sources directory and build them into Mach-O binaries
    Dir.glob(File.join(SOURCES_DIR, "**", "*.c")).sort.each do |source_file|
        binary_name = File.basename(source_file, ".c")
        install_directory = binary_name == "launchd" ? "usr/libexec" : "usr/bin"
        output_file = File.join(ROOTFS_DIR, install_directory, binary_name)
        build_macho(source_file, output_file)
    end

    image_path = File.join(BUILD_DIR, "initrd.ext2")
    File.open(image_path, "wb") { |image| image.truncate(32 * 1024 * 1024) }
    command = [
        "mke2fs", "-t", "ext2", "-F", "-b", "1024", "-I", "128",
        "-O", "none", "-d", ROOTFS_DIR, image_path
    ]
    return if system(*command)

    status = $?
    raise "Failed to create ext2 initrd (exit status #{status&.exitstatus || "unknown"})"
end

make_initrd if $PROGRAM_NAME == __FILE__
