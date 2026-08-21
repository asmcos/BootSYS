//! Boot panel — TUI frame with optional classic ANSI-16 color.
//!
//! minicom disables color by default (`-c off`). Use `minicom -c on`.
//! minicom hard-codes classic ANSI 16-color escapes (not 256/truecolor).

use crate::board::BoardInfo;
use crate::console;

const INNER: usize = 52;

const RESET: &str = "\x1b[0m";
const BOLD: &str = "\x1b[1m";
const DIM: &str = "\x1b[2m";
const CYAN: &str = "\x1b[36m";
const YELLOW: &str = "\x1b[33m";
const GREEN: &str = "\x1b[32m";
const WHITE: &str = "\x1b[37m";

/// How to paint the boot panel.
#[derive(Clone, Copy, PartialEq, Eq)]
pub enum ColorMode {
    Off,
    /// Bold / dim only.
    Intensity,
    /// Classic 16-color — best match for `minicom -c on`.
    Ansi16,
    Ansi256,
    TrueColor,
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum Paint {
    Off,
    Intensity,
    Ansi16,
}

struct Style {
    mode: Paint,
}

impl Style {
    fn new(mode: ColorMode) -> Self {
        let mode = match mode {
            ColorMode::Off => Paint::Off,
            ColorMode::Intensity => Paint::Intensity,
            // minicom: use 16-color; map richer modes down to 16 for UART.
            ColorMode::Ansi16 | ColorMode::Ansi256 | ColorMode::TrueColor => Paint::Ansi16,
        };
        Self { mode }
    }

    fn paint(&self, code: &str, s: &str) {
        match self.mode {
            Paint::Off => console::write_str(s),
            Paint::Intensity => {
                // Map color intents to bold/dim.
                if code == DIM {
                    console::write_str(DIM);
                } else {
                    console::write_str(BOLD);
                }
                console::write_str(s);
                console::write_str(RESET);
            }
            Paint::Ansi16 => {
                console::write_str(code);
                console::write_str(s);
                console::write_str(RESET);
            }
        }
    }

    fn begin(&self, code: &str) {
        match self.mode {
            Paint::Off => {}
            Paint::Intensity => {
                if code == DIM {
                    console::write_str(DIM);
                } else {
                    console::write_str(BOLD);
                }
            }
            Paint::Ansi16 => console::write_str(code),
        }
    }

    fn end(&self) {
        if self.mode != Paint::Off {
            console::write_str(RESET);
        }
    }
}

fn nl() {
    console::write_str("\r\n");
}

fn hline(style: &Style, left: &str, fill: &str, right: &str) {
    style.begin(CYAN);
    console::write_str(left);
    for _ in 0..(INNER + 2) {
        console::write_str(fill);
    }
    console::write_str(right);
    style.end();
    nl();
}

fn pad_rest(used: usize) {
    let mut n = used;
    while n < INNER {
        console::write_str(" ");
        n += 1;
    }
}

fn vbar(style: &Style) {
    style.paint(CYAN, "│");
}

fn hex_digits(mut v: usize) -> usize {
    if v == 0 {
        return 1;
    }
    let mut n = 0;
    while v != 0 {
        v >>= 4;
        n += 1;
    }
    n
}

fn dec_digits(mut v: u32) -> usize {
    if v == 0 {
        return 1;
    }
    let mut n = 0;
    while v != 0 {
        v /= 10;
        n += 1;
    }
    n
}

fn resolve_mode(info: &BoardInfo) -> ColorMode {
    if !info.ansi_color {
        ColorMode::Off
    } else {
        info.color_mode
    }
}

/// Print a TUI-like boot panel.
pub fn print_boot_panel(info: &BoardInfo) {
    let style = Style::new(resolve_mode(info));

    nl();
    hline(&style, "╭", "─", "╮");

    vbar(&style);
    console::write_str(" ");
    style.paint(YELLOW, "◆");
    console::write_str("  ");
    style.begin(BOLD);
    style.begin(CYAN);
    console::write_str("BootSYS");
    style.end();
    pad_rest("◆  BootSYS".chars().count());
    console::write_str(" ");
    vbar(&style);
    nl();

    vbar(&style);
    console::write_str(" ");
    style.paint(DIM, "   RISC-V  ·  low-level verification system");
    pad_rest("   RISC-V  ·  low-level verification system".chars().count());
    console::write_str(" ");
    vbar(&style);
    nl();

    hline(&style, "├", "─", "┤");

    {
        let label = "▸ board     ";
        vbar(&style);
        console::write_str(" ");
        style.paint(YELLOW, "▸");
        console::write_str(" board     ");
        style.begin(BOLD);
        style.begin(WHITE);
        console::write_str(info.name);
        style.end();
        pad_rest(label.chars().count() + info.name.chars().count());
        console::write_str(" ");
        vbar(&style);
        nl();
    }

    {
        let label = "▸ uart      0x";
        vbar(&style);
        console::write_str(" ");
        style.paint(YELLOW, "▸");
        console::write_str(" uart      0x");
        style.begin(CYAN);
        console::write_hex_usize(info.uart_base);
        style.end();
        pad_rest(label.chars().count() + hex_digits(info.uart_base));
        console::write_str(" ");
        vbar(&style);
        nl();
    }

    {
        let label = "▸ baud      ";
        vbar(&style);
        console::write_str(" ");
        style.paint(YELLOW, "▸");
        console::write_str(" baud      ");
        style.begin(CYAN);
        console::write_u32(info.uart_baud);
        style.end();
        pad_rest(label.chars().count() + dec_digits(info.uart_baud));
        console::write_str(" ");
        vbar(&style);
        nl();
    }

    {
        let visible = "▸ status    ● ready";
        vbar(&style);
        console::write_str(" ");
        style.paint(YELLOW, "▸");
        console::write_str(" status    ");
        style.paint(GREEN, "● ready");
        pad_rest(visible.chars().count());
        console::write_str(" ");
        vbar(&style);
        nl();
    }

    hline(&style, "╰", "─", "╯");
    nl();
}

/// Minimal fallback panel.
pub fn print_logo() {
    let style = Style::new(ColorMode::Ansi16);
    nl();
    hline(&style, "╭", "─", "╮");
    vbar(&style);
    console::write_str(" ");
    style.paint(YELLOW, "◆");
    console::write_str("  ");
    style.begin(BOLD);
    style.begin(CYAN);
    console::write_str("BootSYS");
    style.end();
    pad_rest("◆  BootSYS".chars().count());
    console::write_str(" ");
    vbar(&style);
    nl();
    hline(&style, "╰", "─", "╯");
    nl();
}
