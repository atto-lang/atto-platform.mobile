use std::ptr;
unsafe extern "C" {fn ores_mobile_scratch_requirement(length: usize, required:*mut usize)->i32;}
#[test] fn scratch_boundaries(){for n in [0,1,1024,65_535,65_536]{let mut output=usize::MAX;assert_eq!(unsafe{ores_mobile_scratch_requirement(n,&mut output)},0);assert_eq!(output,n);}let mut out=999;assert_eq!(unsafe{ores_mobile_scratch_requirement(65_537,&mut out)},2);assert_eq!(out,0);assert_eq!(unsafe{ores_mobile_scratch_requirement(0,ptr::null_mut())},1);}

use std::ffi::c_void;
#[repr(C)] struct CallbackState { called: usize, valid: bool }
extern "C" fn snapshot_callback(ctx: *mut c_void, data: *const u8, length: usize) -> i32 {
    let state = unsafe { &mut *(ctx as *mut CallbackState) };
    state.called += 1;
    if length == 0 {
        state.valid &= data.is_null();
    } else if data.is_null() {
        state.valid = false;
    } else {
        let bytes = unsafe { std::slice::from_raw_parts(data, length) };
        state.valid &= bytes.iter().enumerate().all(
            |(i, value)| *value == ((i * 17 + 3) % 251) as u8);
    }
    0
}
unsafe extern "C" {
    fn ores_mobile_dispatch_copy(handler: Option<extern "C" fn(*mut c_void,*const u8,usize)->i32>,
        context: *mut c_void, data: *const u8, length: usize,
        scratch: *mut u8, capacity: usize) -> i32;
}
#[test]
fn planned_scratch_matches_real_scrubbed_dispatch() {
    for length in [0usize, 1, 1024, 65535, 65536] {
        let source: Vec<u8> = (0..length).map(|i| ((i * 17 + 3) % 251) as u8).collect();
        let mut scratch = vec![0xa5u8; length + 3];
        let mut required = usize::MAX;
        assert_eq!(unsafe { ores_mobile_scratch_requirement(length, &mut required) }, OK);
        assert_eq!(required, length);
        let mut state = CallbackState { called: 0, valid: true };
        assert_eq!(unsafe { ores_mobile_dispatch_copy(Some(snapshot_callback),
            (&mut state as *mut CallbackState).cast(), source.as_ptr(), length,
            scratch.as_mut_ptr(), scratch.len()) }, OK);
        assert!(state.valid);
        assert_eq!(state.called, 1);
        assert!(scratch[..length].iter().all(|byte| *byte == 0));
        assert!(scratch[length..].iter().all(|byte| *byte == 0xa5));
    }
    let mut state = CallbackState { called: 0, valid: true };
    let mut scratch = [0xa5u8; 4];
    assert_eq!(unsafe { ores_mobile_dispatch_copy(Some(snapshot_callback),
        (&mut state as *mut CallbackState).cast(), scratch.as_ptr(), 65537,
        scratch.as_mut_ptr(), scratch.len()) }, TOO_LARGE);
    assert_eq!(state.called, 0);
    assert_eq!(scratch, [0xa5u8; 4]);
}
