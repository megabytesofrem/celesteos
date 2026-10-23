require "tmpdir"
require "open3"
require "shellwords"
require_relative "../scripts/make_syscalls"

def check(condition, message)
    raise message unless condition
end

root = File.expand_path("..", __dir__)
calls, includes = parse_master_table(File.read(File.join(root, "kernel/syscalls.master")))
header = generate_header(calls, includes, "")
dispatch = generate_dispatch(calls)
check(!header.include?("struct sys_"), "argument structs remain in generated declarations")
check(!dispatch.include?("struct sys_"), "argument structs remain in generated dispatch")
check(header.include?("void sys_exit(int rval);"), "exit needs a direct argument")
check(header.include?("int sys_fork(void);"), "no-argument calls need a void prototype")
check(header.include?("intptr_t sys_write(int fd, const void * buf, size_t nbyte);"),
      "write needs typed direct arguments")
check(dispatch.include?("(mach_port_name_t)stack_args[0], (mach_msg_header_t *)stack_args[1], (mach_msg_size_t)stack_args[2]"),
      "Mach stack argument mapping changed")

fixture_calls, fixture_includes = parse_master_table(<<~TABLE)
    0 STD { int zero(void); }
    1 STD { void finish(int status); }
    2 STD { intptr_t collect(int a, unsigned int b, uintptr_t c, uintptr_t d, uintptr_t e, uintptr_t f, uintptr_t g); }
    3 STD { int missing(void); }
TABLE

Dir.mktmpdir("celesteos-syscalls-") do |directory|
    File.write(File.join(directory, "sys_enter.h"), generate_header(fixture_calls, fixture_includes, ""))
    File.write(File.join(directory, "sys_dispatch.h"), generate_dispatch_header(fixture_calls))
    File.write(File.join(directory, "sys_dispatch.c"), generate_dispatch(fixture_calls))
    harness = File.join(directory, "test.c")
    File.write(harness, <<~C)
        #include "sys_enter.h"
        #include <kernel/memory/pmap.h>
        #include <libkernel.h>
        #include <assert.h>
        #include <string.h>

        static int exit_status;
        static uintptr_t expected_source;
        static int copy_failure;
        int sys_zero(void) { return 42; }
        void sys_finish(int status) { exit_status = status; }
        intptr_t sys_collect(int a, unsigned int b, uintptr_t c, uintptr_t d,
                             uintptr_t e, uintptr_t f, uintptr_t g) {
            assert(a == -1 && b == 2 && c == 3 && d == 4);
            assert(e == 5 && f == 6 && g == 7);
            return 123;
        }
        void klog(enum log_level level, const char *format, ...) {
            (void)level;
            (void)format;
        }
        int pmap_copy_from_user(void *destination, uintptr_t source, size_t size) {
            assert(source == expected_source && size == sizeof(uint64_t));
            if (copy_failure) return -1;
            memcpy(destination, (const void *)source, size);
            return 0;
        }
        int main(void) {
            uint64_t stack[] = {0, 7};
            expected_source = (uintptr_t)&stack[1];
            stack_frame_t frame = {0};
            frame.rdi = UINT64_MAX;
            frame.rsi = 2; frame.rdx = 3; frame.r10 = 4;
            frame.r8 = 5; frame.r9 = 6;
            frame.rsp = (uintptr_t)stack;
            assert(sys_dispatch(SYS_BSD_OFFSET, &frame) == 42);
            assert(sys_dispatch(SYS_BSD_OFFSET + 1, &frame) == 0);
            assert(exit_status == -1);
            assert(sys_dispatch(SYS_BSD_OFFSET + 2, &frame) == 123);
            copy_failure = 1;
            assert(sys_dispatch(SYS_BSD_OFFSET + 2, &frame) == -1);
            assert(sys_dispatch(SYS_BSD_OFFSET + 3, &frame) == -1);
            return 0;
        }
    C
    executable = File.join(directory, "test")
    command = Shellwords.split(ENV.fetch("CC", "cc")) + [
        "-std=c17", "-Wall", "-Wextra", "-Werror", "-fno-builtin",
        "-I#{directory}", "-I#{root}", "-I#{root}/kernel", "-I#{root}/lib/stdlib",
        File.join(directory, "sys_dispatch.c"), harness, "-o", executable
    ]
    output, status = Open3.capture2e(*command)
    check(status.success?, "generated dispatch failed to compile:\n#{output}")
    output, status = Open3.capture2e(executable)
    check(status.success?, "generated dispatch failed at runtime:\n#{output}")
end

puts "syscall generator tests passed"
