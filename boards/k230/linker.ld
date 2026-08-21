/* ==============================================
 * BootSYS K230 linker script
 * Layout mirrors riscvboot for K230 SRAM bring-up.
 * ============================================== */

OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY {
    ROM (rwx) : ORIGIN = 0x80200000, LENGTH = 128K
    RAM (rwx) : ORIGIN = 0x80220000, LENGTH = 128K
}

SECTIONS {
    .init : {
        KEEP(*(.init))
        KEEP(*(.init.*))
        . = ALIGN(8);
    } > ROM

    .text : {
        *(.text.init)
        *(.text .text.*)
        /* Keep a clear gap before .rodata so a bad jump cannot land in strings. */
        . = ALIGN(64);
        BYTE(0x01); BYTE(0x00); /* c.nop */
        BYTE(0x01); BYTE(0x00);
        . = ALIGN(8);
    } > ROM

    .rodata : {
        . = ALIGN(16);
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
        . = ALIGN(8);
    } > ROM

    .lalign : {
        . = ALIGN(8);
        PROVIDE(__data_load_start = .);
    } > ROM AT> ROM

    .dalign : {
        . = ALIGN(8);
        PROVIDE(__data_start = .);
    } > RAM AT> ROM

    .data : {
        _data_start = .;
        *(.data .data.*)
        *(.sdata .sdata.*)
        PROVIDE(__global_pointer$ = . + 0x800);
        _data_end = .;
        . = ALIGN(8);
    } > RAM AT> ROM

    PROVIDE(__data_end = .);
    PROVIDE(edata = .);

    .bss (NOLOAD) : {
        _bss_start = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
        _bss_end = .;
        . = ALIGN(16);
    } > RAM

    /*
     * Heap grows up; stack sits at RAM top growing down.
     * Leave a 4 KiB guard between heap end and stack base.
     */
    .heap (NOLOAD) : {
        . = ALIGN(16);
        _heap_start = .;
        . = ORIGIN(RAM) + LENGTH(RAM) - 0x1000 - 0x1000;
        _heap_end = .;
    } > RAM

    .stack (NOLOAD) : {
        . = ALIGN(16);
        . = ORIGIN(RAM) + LENGTH(RAM) - 0x1000;
        _stack_start = .;
        . += 0x1000;
        _stack_end = .;
        PROVIDE(_sp = .);
    } > RAM

    _end = .;
    _memory_start = ORIGIN(RAM);
    _memory_end = ORIGIN(RAM) + LENGTH(RAM);

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note.*)
        *(.comment)
    }
}
