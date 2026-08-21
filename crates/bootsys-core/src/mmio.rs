//! Volatile MMIO register access helpers.

use core::ptr::{read_volatile, write_volatile};

/// Read a 32-bit MMIO register.
#[inline(always)]
pub unsafe fn read32(addr: usize) -> u32 {
    read_volatile(addr as *const u32)
}

/// Write a 32-bit MMIO register.
#[inline(always)]
pub unsafe fn write32(addr: usize, value: u32) {
    write_volatile(addr as *mut u32, value);
}

/// Read an 8-bit MMIO register (UART data path often uses byte lanes).
#[inline(always)]
pub unsafe fn read8(addr: usize) -> u8 {
    read_volatile(addr as *const u8)
}

/// Write an 8-bit MMIO register.
#[inline(always)]
pub unsafe fn write8(addr: usize, value: u8) {
    write_volatile(addr as *mut u8, value);
}
