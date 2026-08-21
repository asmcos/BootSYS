//! Generic 16550-compatible UART (DW / NS16550 programming model).
//!
//! Base address and input clock come from [`crate::board::BoardInfo`], not
//! from hard-coded SoC constants in the framework.

use crate::mmio::{read32, write32};
use crate::uart::Uart;

const RBR: usize = 0x00;
const THR: usize = 0x00;
const DLL: usize = 0x00;
const DLM: usize = 0x04;
const IER: usize = 0x04;
const FCR: usize = 0x08;
const LCR: usize = 0x0c;
const MCR: usize = 0x10;
const LSR: usize = 0x14;

const LCR_DLAB: u32 = 1 << 7;
const LCR_WLEN_8: u32 = 0x03;

const LSR_DR: u32 = 1 << 0;
const LSR_THRE: u32 = 1 << 5;
const LSR_TEMT: u32 = 1 << 6;

const FCR_FIFO_EN: u32 = 1 << 0;
const FCR_RX_RST: u32 = 1 << 1;
const FCR_TX_RST: u32 = 1 << 2;

#[inline(always)]
fn spin_wait() {
    // Avoid `core::hint::spin_loop()` (`pause`) on early bring-up cores.
    core::hint::black_box(());
}

/// NS16550-style UART driven from board-provided base + clock.
pub struct Ns16550 {
    base: usize,
    clock_hz: u32,
    default_baud: u32,
}

impl Ns16550 {
    pub const fn new(base: usize, clock_hz: u32, default_baud: u32) -> Self {
        Self {
            base,
            clock_hz,
            default_baud,
        }
    }

    pub const fn base(&self) -> usize {
        self.base
    }

    #[inline(always)]
    unsafe fn reg_read(&self, offset: usize) -> u32 {
        read32(self.base + offset)
    }

    #[inline(always)]
    unsafe fn reg_write(&self, offset: usize, value: u32) {
        write32(self.base + offset, value);
    }

    fn divisor(&self, baud: u32) -> u32 {
        let baud = if baud == 0 { self.default_baud } else { baud };
        let mut div = self.clock_hz / (16 * baud);
        if div == 0 {
            div = 1;
        }
        if div > 0xffff {
            div = 0xffff;
        }
        div
    }
}

impl Uart for Ns16550 {
    fn init(&mut self, baud: u32) {
        unsafe {
            self.reg_write(IER, 0);
            self.reg_write(FCR, FCR_FIFO_EN | FCR_RX_RST | FCR_TX_RST);
            self.reg_write(LCR, LCR_DLAB);

            let div = self.divisor(baud);
            self.reg_write(DLL, div & 0xff);
            self.reg_write(DLM, (div >> 8) & 0xff);

            self.reg_write(LCR, LCR_WLEN_8);
            self.reg_write(MCR, 0);

            for _ in 0..1000 {
                spin_wait();
            }
            let _ = self.reg_read(LSR);
            let _ = self.reg_read(RBR);
        }
    }

    fn putc(&mut self, c: u8) {
        unsafe {
            while self.reg_read(LSR) & LSR_THRE == 0 {
                spin_wait();
            }
            self.reg_write(THR, c as u32);
        }
    }

    fn getc(&mut self) -> u8 {
        unsafe {
            while self.reg_read(LSR) & LSR_DR == 0 {
                spin_wait();
            }
            self.reg_read(RBR) as u8
        }
    }

    fn try_getc(&mut self) -> Option<u8> {
        unsafe {
            if self.reg_read(LSR) & LSR_DR != 0 {
                Some(self.reg_read(RBR) as u8)
            } else {
                None
            }
        }
    }

    fn tx_flush(&mut self) {
        unsafe {
            while self.reg_read(LSR) & LSR_TEMT == 0 {
                spin_wait();
            }
        }
    }
}
