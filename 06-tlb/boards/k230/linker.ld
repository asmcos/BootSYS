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

    /* 4K-aligned page tables + probe page */
    .pgtbl (NOLOAD) : {
        . = ALIGN(4096);
        *(.pgtbl .pgtbl.*)
        . = ALIGN(4096);
    } > RAM

    .cpu1_shmem (NOLOAD) : {
        . = ALIGN(8);
        PROVIDE(_cpu1_shmem = .);
        . += 64;
        . = ALIGN(16);
    } > RAM

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

    .heap (NOLOAD) : {
        . = ALIGN(16);
        _heap_start = .;
        . = ORIGIN(RAM) + LENGTH(RAM) - 0x2000;
        _heap_end = .;
    } > RAM

    .stack (NOLOAD) : {
        . = ALIGN(16);
        /* Same as k230-minimal: _sp1 then _sp, 4K each */
        . = ORIGIN(RAM) + LENGTH(RAM) - 0x2000;
        _stack1_start = .;
        . += 0x1000;
        PROVIDE(_sp1 = .);
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
