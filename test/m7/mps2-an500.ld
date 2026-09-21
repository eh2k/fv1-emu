/* QEMU "mps2-an500": Cortex-M7, ZBT SSRAM1 at 0, ZBT SSRAM2&3 at 0x20000000. */
MEMORY
{
    CODE (rx)  : ORIGIN = 0x00000000, LENGTH = 4M
    DATA (rwx) : ORIGIN = 0x20000000, LENGTH = 4M
}

ENTRY(Reset_Handler)

SECTIONS
{
    .vectors : { KEEP(*(.vectors)) } > CODE

    .text : {
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
        . = ALIGN(4);
    } > CODE

    .ARM.exidx : { *(.ARM.exidx*) } > CODE

    __data_load = LOADADDR(.data);
    .data : {
        __data_start = .;
        *(.data*)
        . = ALIGN(4);
        __data_end = .;
    } > DATA AT > CODE

    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(8);
        __bss_end = .;
    } > DATA

    PROVIDE(end = __bss_end);   /* newlib/libnosys _sbrk fallback */
    PROVIDE(_end = __bss_end);
    __heap_start = __bss_end;
    __heap_end   = ORIGIN(DATA) + LENGTH(DATA) - 0x8000;
    __stack_top  = ORIGIN(DATA) + LENGTH(DATA);

    /DISCARD/ : { *(.note*) *(.comment) }
}
