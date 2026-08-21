//! Minimal UART driver interface.

/// Blocking UART used for early bring-up and verification logs.
pub trait Uart {
    fn init(&mut self, baud: u32);
    fn putc(&mut self, c: u8);
    fn getc(&mut self) -> u8;
    fn try_getc(&mut self) -> Option<u8>;
    fn tx_flush(&mut self);

    fn puts(&mut self, s: &str) {
        for b in s.bytes() {
            if b == b'\n' {
                self.putc(b'\r');
            }
            self.putc(b);
        }
    }
}
