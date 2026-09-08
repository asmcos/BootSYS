/* DDR load — use after DRAM is up (U-Boot or your DDR init). */
OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY {
    ROM (rwx) : ORIGIN = 0x50000000, LENGTH = 128K
    RAM (rwx) : ORIGIN = 0x50200000, LENGTH = 128K
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
        *(.rodata .rodata.*)
        . = ALIGN(8);
    } > ROM

    .lalign : {
        . = ALIGN(8);
        PROVIDE(__data_load_start = .);
    } > ROM

    .pmp_secret (NOLOAD) : {
        . = ALIGN(4096);
        PROVIDE(_pmp_secret = .);
        . += 4096;
        PROVIDE(_pmp_secret_end = .);
    } > RAM

    .dalign : {
        . = ALIGN(8);
        PROVIDE(__data_start = .);
    } > RAM AT> ROM

    .data : {
        *(.data .data.*)
        PROVIDE(__data_end = .);
        . = ALIGN(8);
    } > RAM AT> ROM

    .bss (NOLOAD) : {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        _bss_end = .;
        . = ALIGN(16);
    } > RAM

    .stack (NOLOAD) : {
        . = ALIGN(16);
        . = ORIGIN(RAM) + LENGTH(RAM) - 0x2000;
        . += 0x1000;
        PROVIDE(_sp1 = .);
        . += 0x1000;
        PROVIDE(_sp = .);
    } > RAM
}
