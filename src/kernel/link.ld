ENTRY(start)

SECTIONS
{
    . = 1M;

    kernel_start = .;

    .text : ALIGN(4K)
    {
        *(.multiboot)
        *(.bootstrap)
        *(.text)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata*)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    kernel_end = .;
}
