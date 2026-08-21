//! Minimal early console — avoid `core::fmt` on the bring-up path.
//!
//! Symptom we hit on K230: plain `println!("...")` works, then a later
//! `println!("{}", value)` (full `core::fmt`) never returns / traps with
//! `mepc` inside `.rodata` (PC executing string bytes). Until fmt is fully
//! vetted on this core/load path, early logs use byte puts + a tiny itoa.

use core::fmt::{self, Write};
use core::sync::atomic::{AtomicPtr, Ordering};

use crate::uart::Uart;

/// Console backend wrapping a board UART.
pub struct Console<U: Uart> {
    uart: U,
}

impl<U: Uart> Console<U> {
    pub const fn new(uart: U) -> Self {
        Self { uart }
    }

    pub fn uart_mut(&mut self) -> &mut U {
        &mut self.uart
    }
}

impl<U: Uart> Write for Console<U> {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        self.uart.puts(s);
        Ok(())
    }
}

type WriteFn = unsafe fn(*mut (), *const u8, usize);

#[repr(C)]
struct ConsoleState {
    write: WriteFn,
    ptr: *mut (),
}

static CONSOLE: AtomicPtr<ConsoleState> = AtomicPtr::new(core::ptr::null_mut());

unsafe fn write_console<U: Uart>(ptr: *mut (), bytes: *const u8, len: usize) {
    let console = &mut *(ptr as *mut Console<U>);
    let s = core::str::from_utf8_unchecked(core::slice::from_raw_parts(bytes, len));
    console.uart.puts(s);
}

unsafe fn write_nop(_ptr: *mut (), _bytes: *const u8, _len: usize) {}

/// Register the board console. `console` must remain valid for the system lifetime.
pub unsafe fn set_console<U: Uart>(console: &'static mut Console<U>) {
    static mut STATE: ConsoleState = ConsoleState {
        write: write_nop,
        ptr: core::ptr::null_mut(),
    };

    STATE.write = write_console::<U>;
    STATE.ptr = console as *mut Console<U> as *mut ();
    CONSOLE.store(core::ptr::addr_of_mut!(STATE), Ordering::Release);
}

/// Write raw bytes to the registered console (no `core::fmt`).
pub fn write_str(s: &str) {
    let state = CONSOLE.load(Ordering::Acquire);
    if state.is_null() {
        return;
    }
    unsafe {
        let state = &*state;
        (state.write)(state.ptr, s.as_ptr(), s.len());
    }
}

/// Write an unsigned decimal integer (no `core::fmt`).
pub fn write_u32(mut v: u32) {
    let mut buf = [0u8; 10];
    let mut i = buf.len();
    if v == 0 {
        write_str("0");
        return;
    }
    while v != 0 {
        i -= 1;
        buf[i] = b'0' + (v % 10) as u8;
        v /= 10;
    }
    unsafe {
        write_str(core::str::from_utf8_unchecked(&buf[i..]));
    }
}

pub fn write_line(s: &str) {
    write_str(s);
    write_str("\r\n");
}

/// Write a lowercase hex integer without `0x` prefix (no `core::fmt`).
pub fn write_hex_usize(mut v: usize) {
    let mut buf = [b'0'; 16];
    let mut i = buf.len();
    if v == 0 {
        write_str("0");
        return;
    }
    while v != 0 {
        i -= 1;
        let n = (v & 0xf) as u8;
        buf[i] = if n < 10 { b'0' + n } else { b'a' + (n - 10) };
        v >>= 4;
    }
    unsafe {
        write_str(core::str::from_utf8_unchecked(&buf[i..]));
    }
}

struct ConsoleWriter;

impl Write for ConsoleWriter {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        write_str(s);
        Ok(())
    }
}

#[doc(hidden)]
pub fn _print(args: fmt::Arguments) {
    let _ = ConsoleWriter.write_fmt(args);
}

#[macro_export]
macro_rules! print {
    ($s:literal) => {{
        // Literal-only, no newline — safe early bring-up path.
        $crate::console::write_str($s);
    }};
    ($($arg:tt)*) => {{
        // Formatted path — uses `core::fmt` (avoid in earliest bring-up).
        $crate::console::_print(core::format_args!($($arg)*));
    }};
}

#[macro_export]
macro_rules! println {
    () => {{
        $crate::console::write_str("\r\n");
    }};
    ($s:literal) => {{
        // Literal-only: never touch `core::fmt` (safe early bring-up path).
        $crate::console::write_line($s);
    }};
    ($($arg:tt)*) => {{
        // Formatted path — uses `core::fmt` (heavier; avoid in earliest bring-up).
        $crate::console::_print(core::format_args!($($arg)*));
        $crate::console::write_str("\r\n");
    }};
}
