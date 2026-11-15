use std::ptr;
unsafe extern "C" {fn ores_mobile_scratch_requirement(length: usize, required:*mut usize)->i32;}
#[test] fn scratch_boundaries(){for n in [0,1,1024,65_535,65_536]{let mut output=usize::MAX;assert_eq!(unsafe{ores_mobile_scratch_requirement(n,&mut output)},0);assert_eq!(output,n);}let mut out=999;assert_eq!(unsafe{ores_mobile_scratch_requirement(65_537,&mut out)},2);assert_eq!(out,0);assert_eq!(unsafe{ores_mobile_scratch_requirement(0,ptr::null_mut())},1);}
