//! Canaan K230 board information only.
//!
//! Memory layout: `linker.ld`. Framework `main` and `arch/riscv/boot.S` are shared.

#![no_std]

use bootsys_core::{Board, BoardInfo, ColorMode};

/// K230 board marker used by the BootSYS framework feature `board-k230`.
pub struct K230Board;

impl Board for K230Board {
    fn info() -> BoardInfo {
        BoardInfo {
            name: "K230",
            uart_base: 0x9140_0000,
            uart_clock_hz: 48_600_000,
            uart_baud: 115_200,
            ansi_color: true,
            // Serial tools often ignore classic 16-color; use truecolor like modern TUIs.
            color_mode: ColorMode::TrueColor,
        }
    }
}
