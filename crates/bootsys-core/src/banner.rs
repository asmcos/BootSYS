//! Boot panel — TUI-style frame with icons + modern ANSI color.
//!
//! Many serial tools ignore classic 16-color codes (`\x1b[32m`) but still
//! honor bold. OpenCode-style TUIs use 256-color / truecolor — we do the same:
//! `\x1b[38;5;Nm` and `\x1b[38;2;r;g;bm`.

use crate::board::BoardInfo;
use crate::console;

const INNER: usize = 52;

const RESET: &str = "\x1b[0m";

/// OpenCode-like palette (truecolor). Falls back paths still reset cleanly.
mod tc {
    pub const BORDER: &str = "\x1b[38;2;34;211;238m"; // cyan-400
    pub const TITLE: &str = "\x1b[1;38;2;165;243;252m"; // bold cyan-200
    pub const ICON: &str = "\x1b[1;38;2;251;191;36m"; // bold amber-400
    pub const MUTED: &str = "\x1b[38;2;148;163;184m"; // slate-400
    pub const VALUE: &str = "\x1b[1;38;2;240;249;255m"; // bold almost-white
    pub const ADDR: &str = "\x1b[38;2;103;232;249m"; // cyan-300
    pub const OK: &str = "\x1b[1;38;2;74;222;128m"; // bold green-400
}

/// 256-color fallback (widely supported in modern terminals).
mod c256 {
    pub const BORDER: &str = "\x1b[38;5;51m";
    pub const TITLE: &str = "\x1b[1;38;5;159m";
    pub const ICON: &str = "\x1b[1;38;5;221m";
    pub const MUTED: &str = "\x1b[38;5;145m";
    pub const VALUE: &str = "\x1b[1;38;5;255m";
    pub const ADDR: &str = "\x1b[38;5;87m";
    pub const OK: &str = "\x1b[1;38;5;120m";
}

/// How to paint the boot panel.
#[derive(Clone, Copy, PartialEq, Eq)]
pub enum ColorMode {
    /// No escapes.
    Off,
    /// Classic 16-color (`\x1b[3Xm`) — often looks B/W on serial tools.
    Ansi16,
    /// xterm 256-color.
    Ansi256,
    /// 24-bit truecolor (closest to modern TUI apps).
    TrueColor,
}

#[derive(Clone, Copy)]
struct Palette {
    border: &'static str,
    title: &'static str,
    icon: &'static str,
    muted: &'static str,
    value: &'static str,
    addr: &'static str,
    ok: &'static str,
}

impl Palette {
    fn for_mode(mode: ColorMode) -> Option<Self> {
        match mode {
            ColorMode::Off => None,
            ColorMode::Ansi16 => Some(Self {
                border: "\x1b[36m",
                title: "\x1b[1;36m",
                icon: "\x1b[1;33m",
                muted: "\x1b[2m",
                value: "\x1b[1;37m",
                addr: "\x1b[36m",
                ok: "\x1b[1;32m",
            }),
            ColorMode::Ansi256 => Some(Self {
                border: c256::BORDER,
                title: c256::TITLE,
                icon: c256::ICON,
                muted: c256::MUTED,
                value: c256::VALUE,
                addr: c256::ADDR,
                ok: c256::OK,
            }),
            ColorMode::TrueColor => Some(Self {
                border: tc::BORDER,
                title: tc::TITLE,
                icon: tc::ICON,
                muted: tc::MUTED,
                value: tc::VALUE,
                addr: tc::ADDR,
                ok: tc::OK,
            }),
        }
    }
}

struct Style {
    pal: Option<Palette>,
}

impl Style {
    fn new(mode: ColorMode) -> Self {
        Self {
            pal: Palette::for_mode(mode),
        }
    }

    fn paint(&self, code: &str, s: &str) {
        if let Some(_) = self.pal {
            console::write_str(code);
            console::write_str(s);
            console::write_str(RESET);
        } else {
            console::write_str(s);
        }
    }

    fn begin(&self, code: &str) {
        if self.pal.is_some() {
            console::write_str(code);
        }
    }

    fn end(&self) {
        if self.pal.is_some() {
            console::write_str(RESET);
        }
    }

    fn p(&self) -> Palette {
        match self.pal {
            Some(p) => p,
            None => Palette {
                border: "",
                title: "",
                icon: "",
                muted: "",
                value: "",
                addr: "",
                ok: "",
            },
        }
    }
}

fn nl() {
    console::write_str("\r\n");
}

fn hline(style: &Style, left: &str, fill: &str, right: &str) {
    let p = style.p();
    style.begin(p.border);
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
    style.paint(style.p().border, "│");
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
        return ColorMode::Off;
    }
    info.color_mode
}

/// Print a TUI-like boot panel (icons + box drawing + modern color).
pub fn print_boot_panel(info: &BoardInfo) {
    let style = Style::new(resolve_mode(info));
    let p = style.p();

    nl();
    hline(&style, "╭", "─", "╮");

    // Title
    vbar(&style);
    console::write_str(" ");
    style.paint(p.icon, "◆");
    console::write_str("  ");
    style.paint(p.title, "BootSYS");
    pad_rest("◆  BootSYS".chars().count());
    console::write_str(" ");
    vbar(&style);
    nl();

    // Subtitle
    vbar(&style);
    console::write_str(" ");
    style.paint(p.muted, "   RISC-V  ·  low-level verification system");
    pad_rest("   RISC-V  ·  low-level verification system".chars().count());
    console::write_str(" ");
    vbar(&style);
    nl();

    hline(&style, "├", "─", "┤");

    // board
    {
        let label = "▸ board     ";
        vbar(&style);
        console::write_str(" ");
        style.paint(p.icon, "▸");
        console::write_str(" board     ");
        style.paint(p.value, info.name);
        pad_rest(label.chars().count() + info.name.chars().count());
        console::write_str(" ");
        vbar(&style);
        nl();
    }

    // uart
    {
        let label = "▸ uart      0x";
        vbar(&style);
        console::write_str(" ");
        style.paint(p.icon, "▸");
        console::write_str(" uart      0x");
        style.begin(p.addr);
        console::write_hex_usize(info.uart_base);
        style.end();
        pad_rest(label.chars().count() + hex_digits(info.uart_base));
        console::write_str(" ");
        vbar(&style);
        nl();
    }

    // baud
    {
        let label = "▸ baud      ";
        vbar(&style);
        console::write_str(" ");
        style.paint(p.icon, "▸");
        console::write_str(" baud      ");
        style.begin(p.addr);
        console::write_u32(info.uart_baud);
        style.end();
        pad_rest(label.chars().count() + dec_digits(info.uart_baud));
        console::write_str(" ");
        vbar(&style);
        nl();
    }

    // status
    {
        let visible = "▸ status    ● ready";
        vbar(&style);
        console::write_str(" ");
        style.paint(p.icon, "▸");
        console::write_str(" status    ");
        style.paint(p.ok, "● ready");
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
    let style = Style::new(ColorMode::TrueColor);
    let p = style.p();
    nl();
    hline(&style, "╭", "─", "╮");
    vbar(&style);
    console::write_str(" ");
    style.paint(p.icon, "◆");
    console::write_str("  ");
    style.paint(p.title, "BootSYS");
    pad_rest("◆  BootSYS".chars().count());
    console::write_str(" ");
    vbar(&style);
    nl();
    hline(&style, "╰", "─", "╯");
    nl();
}
