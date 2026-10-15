cmake_minimum_required(VERSION 3.20)

# Enable C, C++, and NASM Assembly languages
project(CelesteOS LANGUAGES C ASM_NASM)

# Enforce C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Configure NASM for 64-bit ELF output format
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_ASM_NASM_FLAGS_DEBUG "-g -F dwarf")

# -----------------------------------------------------------------------------
# Bare-Metal C Compiler Flags
# -----------------------------------------------------------------------------
set(KERNEL_FLAGS
    "-ffreestanding"
    "-mno-red-zone"
    "-mcmodel=kernel"
    "-mno-mmx"
    "-mno-sse2"
    "-fno-stack-protector"
    "-Wall"
    "-Wno-unused-variable"
    "-Wextra"
)

# Safely join list items with spaces instead of embedding multi-line newlines
list(JOIN KERNEL_FLAGS " " KERNEL_FLAGS_STR)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${KERNEL_FLAGS_STR}")

# -----------------------------------------------------------------------------
# Executable Definition
# -----------------------------------------------------------------------------
add_executable(celesteos.elf
    src/kernel/kmain.c
    src/kernel/cpu/gdt.c
    src/kernel/cpu/load_gdt.asm
    src/kernel/cpu/idt.c
    src/kernel/memory/pmm.c
    src/kernel/memory/vmm.c
    src/kernel/memory/heap_alloc.c


    # Standard library implementations
    src/stdlib/string.c
    src/stdlib/libkernel.c
    src/kernel/drivers/framebuffer.c
    src/kernel/drivers/ter_u16b.h
    src/kernel/macho.c
)

# Include directory setup for kernel headers
file(GLOB_RECURSE KERNEL_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/*.h"
)

set(KERNEL_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/kernel")
foreach(KERNEL_HEADER IN LISTS KERNEL_HEADERS)
    get_filename_component(KERNEL_HEADER_DIR "${KERNEL_HEADER}" DIRECTORY)
    list(APPEND KERNEL_INCLUDE_DIRS "${KERNEL_HEADER_DIR}")
endforeach()
list(REMOVE_DUPLICATES KERNEL_INCLUDE_DIRS)

target_include_directories(celesteos.elf PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/stdlib"
    ${KERNEL_INCLUDE_DIRS}
)

# -----------------------------------------------------------------------------
# Linker Settings & Script Target
# -----------------------------------------------------------------------------
set_target_properties(celesteos.elf PROPERTIES
    LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -nostdlib -static"
)

# Link freestanding support libraries (libgcc for 64-bit integer division/stubs)
target_link_libraries(celesteos.elf PRIVATE gcc)

# -----------------------------------------------------------------------------
# Limine ISO Packaging Target
# -----------------------------------------------------------------------------
set(ISO_ROOT "${CMAKE_BINARY_DIR}/iso_root")
set(LIMINE_DIR "${CMAKE_BINARY_DIR}/limine")

find_program(BASH_EXECUTABLE bash REQUIRED)
find_program(XORRISO_EXECUTABLE xorriso REQUIRED)

add_custom_target(limine_bootloader
    COMMAND ${CMAKE_COMMAND} -E env "LIMINE_DIR=${LIMINE_DIR}"
            "${BASH_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_limine.sh"
    COMMENT "Preparing Limine boot files and host installer..."
    VERBATIM
)

add_custom_target(iso ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ISO_ROOT}/boot"
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:celesteos.elf> "${ISO_ROOT}/boot/kernel.elf"
    COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_DIR}/limine-bios.sys ${ISO_ROOT}/
    COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_DIR}/limine-bios-cd.bin ${ISO_ROOT}/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/limine.conf ${ISO_ROOT}/
    COMMAND "${XORRISO_EXECUTABLE}" -as mkisofs -b limine-bios-cd.bin
            -no-emul-boot -boot-load-size 4 -boot-info-table
            ${ISO_ROOT} -o ${CMAKE_BINARY_DIR}/celesteos.iso
    COMMAND ${LIMINE_DIR}/limine bios-install ${CMAKE_BINARY_DIR}/celesteos.iso
    DEPENDS celesteos.elf limine_bootloader
    COMMENT "Packaging bootable ISO image with Limine..."
    VERBATIM
)

# -----------------------------------------------------------------------------
# QEMU launch target
# -----------------------------------------------------------------------------
find_program(QEMU_EXECUTABLE qemu-system-x86_64 REQUIRED)

add_custom_target(qemu
    COMMAND "${QEMU_EXECUTABLE}"
            -cdrom "${CMAKE_BINARY_DIR}/celesteos.iso"
            -m 512M
            -debugcon stdio
            -no-reboot
    DEPENDS iso
    COMMENT "Launching CelesteOS in QEMU..."
    USES_TERMINAL
)