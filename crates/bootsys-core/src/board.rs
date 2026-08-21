//! Board description consumed by the BootSYS framework.
//!
//! Board crates only supply this information (name, UART, clocks, memory).
//! They do not own `main` or the RISC-V boot path.

/// Static board / SoC parameters for bring-up and verification.
#[derive(Clone, Copy)]
pub struct BoardInfo {
    /// Human-readable board name, e.g. `"K230"`.
    pub name: &'static str,
    /// Console UART MMIO base address.
    pub uart_base: usize,
    /// UART input clock in Hz (for baud divisor).
    pub uart_clock_hz: u32,
    /// Default console baud rate.
    pub uart_baud: u32,
    /// Master switch for colored boot panel.
    pub ansi_color: bool,
    /// Color protocol. Prefer `TrueColor` / `Ansi256` (OpenCode-like).
    /// Classic `Ansi16` often looks black/white on serial tools.
    pub color_mode: crate::banner::ColorMode,
}

/// Implemented by each board crate (`boards/k230`, …).
pub trait Board {
    fn info() -> BoardInfo;
}
