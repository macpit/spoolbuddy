fn main() {
    // Ensure the build is rerun if linker script changes
    println!("cargo:rerun-if-changed=build.rs");
    // Make sure linkall.x is the last linker script (critical for esp-hal)
    println!("cargo:rustc-link-arg=-Tlinkall.x");
}
