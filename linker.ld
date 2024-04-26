OUTPUT_FORMAT(elf64-x86-64)

ENTRY(_start)
SECTIONS
{
    /* Put the kernel at 2 MB */
    kern_virt_offset = 0xffffffff80000000;

    /* Multiboot section */
    . = 0x100000;
    .multiboot ALIGN(4K) : AT(ADDR(.multiboot))
    {
        *(.multiboot)
    }

    /* Begin putting sections at kernel_virt_offset */
    . += kern_virt_offset;
    .text ALIGN(4K) :  AT(ADDR(.text) - kern_virt_offset)
    {
        sections_text = . - kern_virt_offset;
        KEEP(*(.text))
        sections_text_end = . - kern_virt_offset;
    }

    /* Read-only data */
    .rodata ALIGN(4K) :  AT(ADDR(.rodata) - kern_virt_offset)
    {
        sections_rodata = . - kern_virt_offset;
        KEEP(*(.rodata*))
        sections_rodata_end = . - kern_virt_offset;
    }

    /* Read-write data (initialized) */
	.data ALIGN(4K) :  AT(ADDR(.data) - kern_virt_offset)
	{
		sections_data = . - kern_virt_offset;
        KEEP(*(.data*))
        sections_data_end = . - kern_virt_offset;
	}
 
	/* Read-write data (uninitialized) and stack */
	.bss ALIGN(4K) :  AT(ADDR(.bss) - kern_virt_offset)
	{
		sections_bss = . - kern_virt_offset;
        KEEP(*(COMMON))
        KEEP(*(.bss*))
        sections_bss_end = . - kern_virt_offset;
	}

    /* Store the kernel end address */
    __kernel_end = .;
}