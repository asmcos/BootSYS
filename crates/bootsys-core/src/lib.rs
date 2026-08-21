//! BootSYS shared primitives for the RISC-V low-level verification system.
//!
//! BootSYS is **not** an OS or bootloader.

#![no_std]

pub mod banner;
pub mod board;
pub mod console;
pub mod mmio;
pub mod ns16550;
pub mod uart;

pub use banner::{print_boot_panel, print_logo, ColorMode};
pub use board::{Board, BoardInfo};
pub use console::{
    set_console, write_hex_usize, write_line, write_str, write_u32, Console,
};
pub use ns16550::Ns16550;
pub use uart::Uart;
