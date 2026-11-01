#![forbid(unsafe_code)]
//! Two host admission tests run by default. Real compiler tests are opt-in
//! and must not be counted as executed unless a trusted driver actually runs.
use std::fs;
use std::path::{Path, PathBuf};
#[cfg(feature = "real-compiler")]
use std::env;
#[cfg(feature = "real-compiler")]
use std::process::{Command, Stdio};
#[cfg(feature = "real-compiler")]
use std::time::{Duration, Instant};

const FIXTURES: [&str; 3] = [
    "positive-arithmetic.ores",
    "reject-unknown-symbol.ores",
    "reject-type-mismatch.ores",
];
fn repo_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../..")
        .canonicalize()
        .expect("repo root")
}
fn fixture_path(name: &str) -> Result<PathBuf, String> {
    if !FIXTURES.contains(&name) {
        return Err("unlisted fixture".into());
    }
    let path = repo_root().join("tests/compiler").join(name);
    let meta = fs::symlink_metadata(&path).map_err(|e| format!("fixture missing: {e}"))?;
    if meta.file_type().is_symlink() || !meta.is_file() || meta.len() == 0 || meta.len() > 16_384 {
        return Err("fixture not a bounded regular file".into());
    }
    Ok(path)
}
fn driver_path(value: Option<&str>) -> Result<PathBuf, String> {
    let path = PathBuf::from(value.ok_or("ORESLANG_COMPILER_DRIVER is required")?);
    if !path.is_absolute() {
        return Err("driver must be absolute".into());
    }
    let meta = fs::symlink_metadata(&path).map_err(|e| format!("driver unavailable: {e}"))?;
    if meta.file_type().is_symlink() || !meta.is_file() {
        return Err("driver must be a regular, non-symlink file".into());
    }
    Ok(path)
}

#[test]
fn bounded_fixture_inventory() {
    for name in FIXTURES {
        fixture_path(name).unwrap();
    }
}
#[test]
fn driver_admission_fails_closed() {
    assert!(driver_path(None).is_err());
    assert!(driver_path(Some("relative/compiler")).is_err());
    assert!(driver_path(Some("/missing/oreslang/compile-driver")).is_err());
}

#[cfg(feature = "real-compiler")]
fn compile_expect(name: &str, expected_success: bool) {
    let configured = env::var("ORESLANG_COMPILER_DRIVER")
        .expect("real-compiler requires ORESLANG_COMPILER_DRIVER");
    let binary = driver_path(Some(&configured)).expect("invalid compiler driver");
    let fixture = fixture_path(name).expect("invalid fixture");
    // Adapter takes one absolute fixture path and propagates real compiler status.
    // No shell or PATH lookup. Adapters and compilers require independent review.
    let mut child = Command::new(binary)
        .arg(fixture)
        .current_dir(repo_root())
        .stdin(Stdio::null())
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .spawn()
        .expect("cannot launch compiler driver");
    let deadline = Instant::now() + Duration::from_secs(30);
    let status = loop {
        match child.try_wait().expect("compiler wait failed") {
            Some(status) => break status,
            None if Instant::now() >= deadline => {
                let _ = child.kill();
                let _ = child.wait();
                panic!("compiler deadline exceeded (not a diagnostic)");
            }
            None => std::thread::sleep(Duration::from_millis(20)),
        }
    };
    assert!(status.code().is_some(), "compiler terminated by signal");
    assert_eq!(status.success(), expected_success, "unexpected compiler outcome for {name}");
}

#[cfg(feature = "real-compiler")]
#[test]
fn compiler_accepts_scalar_fixture() {
    compile_expect(FIXTURES[0], true);
}
#[cfg(feature = "real-compiler")]
#[test]
fn compiler_rejects_unknown_symbol() {
    compile_expect(FIXTURES[1], false);
}
#[cfg(feature = "real-compiler")]
#[test]
fn compiler_rejects_type_mismatch() {
    compile_expect(FIXTURES[2], false);
}
