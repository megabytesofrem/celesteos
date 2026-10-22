#!/usr/bin/env ruby

require "fileutils"

ROOT_DIR = File.expand_path("..", __dir__)
SCRIPT_DIR = __dir__
TEMPLATE_DIR = File.join(ROOT_DIR, "user/template")
INITRD_DIR = File.join(ROOT_DIR, "user/initrd")
SOURCES_DIR = File.join(ROOT_DIR, "user/sources")
BUILD_DIR = File.join(SCRIPT_DIR, "build")

TEMPLATE_DIRECTORIES = %w[
    System/Library/Frameworks
    System/Library/Extensions
    System/Library/CoreServices
    System/Library/LaunchAgents
    System/Library/LaunchDaemons
    Library
    Library/Application Support
    Library/Preferences
    Library/Preferences/SystemConfiguration
    Library/LaunchAgents
    Library/LaunchDaemons
    Applications
    Volumes
    usr/bin
    usr/lib
    usr/libexec
    private/etc
    private/var
    private/tmp
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
    # Create necessary directories for building the initrd and template files
    FileUtils.mkdir_p([BUILD_DIR, INITRD_DIR, TEMPLATE_DIR])
    FileUtils.mkdir_p(TEMPLATE_DIRECTORIES.map { |path| File.join(TEMPLATE_DIR, path) })

    # Find all *.c source files in the sources directory and build them into Mach-O binaries
    Dir.glob(File.join(SOURCES_DIR, "**", "*.c")).sort.each do |source_file|
        binary_name = File.basename(source_file, ".c")
        install_directory = binary_name == "launchd" ? "usr/libexec" : "usr/bin"
        output_file = File.join(TEMPLATE_DIR, install_directory, binary_name)
        build_macho(source_file, output_file)
    end

    # Copy all template files into the initrd directory before creating the tarball
    Dir.children(TEMPLATE_DIR).each do |entry|
        FileUtils.cp_r(File.join(TEMPLATE_DIR, entry), INITRD_DIR)
    end

    # Create the initrd tarball from the contents of the initrd directory
    command = [
        "tar",
        "--format=ustar",
        "-cf",
        File.join(BUILD_DIR, "initrd.tar"),
        "-C",
        INITRD_DIR,
        "."
    ]
    return if system(*command)

    status = $?
    raise "Failed to create initrd tarball (exit status #{status&.exitstatus || "unknown"})"
end

make_initrd if $PROGRAM_NAME == __FILE__
