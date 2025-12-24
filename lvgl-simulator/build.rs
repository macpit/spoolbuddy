//! Build script for SpoolBuddy LVGL Simulator
//!
//! Compiles the EEZ Studio generated C files for the UI.

use std::env;
use std::path::PathBuf;

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let ui_dir = manifest_dir.join("src/ui");

    // Get LVGL include path from DEP_LV_CONFIG_PATH or use local include
    let lv_config_path = env::var("DEP_LV_CONFIG_PATH")
        .map(PathBuf::from)
        .unwrap_or_else(|_| manifest_dir.join("include"));

    // Collect all C files from src/ui
    let c_files: Vec<_> = std::fs::read_dir(&ui_dir)
        .expect("Failed to read src/ui directory")
        .filter_map(|entry| {
            let entry = entry.ok()?;
            let path = entry.path();
            if path.extension().map_or(false, |ext| ext == "c") {
                Some(path)
            } else {
                None
            }
        })
        .collect();

    if c_files.is_empty() {
        println!("cargo:warning=No C files found in src/ui");
        return;
    }

    println!("cargo:rerun-if-changed=src/ui");
    for file in &c_files {
        println!("cargo:rerun-if-changed={}", file.display());
    }

    // Find LVGL vendor path from cargo git checkout
    let home = env::var("HOME").unwrap_or_else(|_| "/opt/claude".to_string());
    let cargo_git = PathBuf::from(&home).join(".cargo/git/checkouts");

    // Find lv_binding_rust checkout directory
    let mut lvgl_vendor_path = None;
    if let Ok(entries) = std::fs::read_dir(&cargo_git) {
        for entry in entries.flatten() {
            let name = entry.file_name();
            if name.to_string_lossy().starts_with("lv_binding_rust") {
                // Find the actual checkout subdirectory
                if let Ok(sub_entries) = std::fs::read_dir(entry.path()) {
                    for sub_entry in sub_entries.flatten() {
                        let vendor_path = sub_entry.path().join("lvgl-sys/vendor/lvgl");
                        if vendor_path.exists() {
                            lvgl_vendor_path = Some(vendor_path);
                            break;
                        }
                    }
                }
            }
            if lvgl_vendor_path.is_some() {
                break;
            }
        }
    }

    let mut build = cc::Build::new();

    build
        .files(&c_files)
        .include(&ui_dir)
        .include(&lv_config_path)
        .include(&manifest_dir.join("include"))
        .warnings(false)  // EEZ generated code may have warnings
        .opt_level(2);

    // Add LVGL vendor path if found - this provides "lvgl/lvgl.h"
    if let Some(ref vendor_path) = lvgl_vendor_path {
        // Include parent of vendor/lvgl so "lvgl/lvgl.h" works
        if let Some(parent) = vendor_path.parent() {
            build.include(parent);
        }
        build.include(vendor_path);
        println!("cargo:warning=Using LVGL vendor path: {:?}", vendor_path);
    }

    // Also check OUT_DIR for lvgl-sys built artifacts
    if let Ok(out_dir) = env::var("OUT_DIR") {
        let out_path = PathBuf::from(&out_dir);
        println!("cargo:warning=OUT_DIR = {:?}", out_path);

        // Try to find lvgl-sys output in the build directory tree
        let build_dir = out_path.parent()
            .and_then(|p| p.parent())
            .and_then(|p| p.parent());

        if let Some(build_dir) = build_dir {
            // Look for lvgl-sys-* directories
            if let Ok(entries) = std::fs::read_dir(build_dir) {
                for entry in entries.flatten() {
                    let name = entry.file_name();
                    if name.to_string_lossy().starts_with("lvgl-sys-") {
                        let lvgl_out = entry.path().join("out");
                        if lvgl_out.exists() {
                            build.include(&lvgl_out);
                            build.include(lvgl_out.join("lvgl"));
                        }
                    }
                }
            }
        }
    }

    build.compile("eez_ui");

    println!("cargo:rustc-link-lib=static=eez_ui");
}
