//! BootSYS framework entry — board-agnostic verification bring-up.
//!
//! Board name, UART base, and clocks come from the selected board crate.
//! RISC-V start code lives in `arch/riscv/boot.S`.

#![no_std]
#![no_main]

use core::arch::global_asm;
use core::panic::PanicInfo;

use bootsys_core::console::{self, Console};
use bootsys_core::{print_boot_panel, set_console, Board, BoardInfo, Ns16550, Uart};

global_asm!(include_str!("../../../arch/riscv/boot.S"));

#[cfg(feature = "board-k230")]
use bootsys_board_k230::K230Board as ActiveBoard;

type BoardUart = Ns16550;

fn board_info() -> BoardInfo {
    ActiveBoard::info()
}

fn make_uart(info: &BoardInfo) -> BoardUart {
    Ns16550::new(info.uart_base, info.uart_clock_hz, info.uart_baud)
}

static mut CONSOLE: Console<BoardUart> =
    Console::new(Ns16550::new(0, 0, 115_200));

#[no_mangle]
pub extern "C" fn rust_main() -> ! {
    let info = board_info();

    unsafe {
        let console = &mut *core::ptr::addr_of_mut!(CONSOLE);
        *console = Console::new(make_uart(&info));
        console.uart_mut().init(info.uart_baud);
        set_console(console);
    }

    // TUI-style boot panel (Unicode box + icons). Terminal must be UTF-8.
    print_boot_panel(&info);

    loop {
        unsafe {
            let console = &mut *core::ptr::addr_of_mut!(CONSOLE);
            if let Some(c) = console.uart_mut().try_getc() {
                if c == b'\r' {
                    console.uart_mut().putc(b'\n');
                }
                console.uart_mut().putc(c);
            }
        }
    }
}

#[no_mangle]
pub extern "C" fn handle_trap(frame: *mut usize) -> ! {
    let mcause = unsafe { *frame.add(34) };
    let mepc = unsafe { *frame.add(31) };
    let mtval = unsafe { *frame.add(33) };

    console::write_str("\r\nTRAP mcause=0x");
    console::write_hex_usize(mcause);
    console::write_str(" mepc=0x");
    console::write_hex_usize(mepc);
    console::write_str(" mtval=0x");
    console::write_hex_usize(mtval);
    console::write_str("\r\n");

    loop {
        core::hint::black_box(());
    }
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    console::write_line("PANIC");
    loop {
        core::hint::black_box(());
    }
}
