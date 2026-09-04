OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY {
    ROM (rwx) : ORIGIN = 0x80200000, LENGTH = 128K
    RAM (rwx) : ORIGIN = 0x80220000, LENGTH = 128K
}

SECTIONS {
    .init : {
        KEEP(*(.init))
        . = ALIGN(8);
    } > ROM

    .text : {
        *(.text .text.*)
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
        *(.data .data.*)
        *(.sdata .sdata.*)
        PROVIDE(__global_pointer$ = . + 0x800);
        . = ALIGN(8);
    } > RAM AT> ROM

    PROVIDE(__data_end = .);

    .bss (NOLOAD) : {
        _bss_start = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
        _bss_end = .;
        . = ALIGN(16);
    } > RAM

    .stack (NOLOAD) : {
        . = ALIGN(16);
        . = ORIGIN(RAM) + LENGTH(RAM) - 0x1000;
        . += 0x1000;
        PROVIDE(_sp = .);
    } > RAM

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note.*)
        *(.comment)
    }
}
