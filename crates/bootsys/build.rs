//! Select board crate + RISC-V linker script for the active feature.

use std::env;
use std::path::PathBuf;

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let workspace = manifest_dir
        .parent()
        .and_then(|p| p.parent())
        .expect("crates/bootsys -> workspace root");

    let board = if env::var("CARGO_FEATURE_BOARD_K230").is_ok() {
        "k230"
    } else {
        panic!("select a board feature, e.g. --features board-k230");
    };

    let linker = workspace.join("boards").join(board).join("linker.ld");
    let boot = workspace.join("arch").join("riscv").join("boot.S");

    println!("cargo:rustc-link-arg=-T{}", linker.display());
    println!("cargo:rerun-if-changed={}", linker.display());
    println!("cargo:rerun-if-changed={}", boot.display());
    println!("cargo:rustc-env=BOOTSYS_BOARD={}", board);
}
