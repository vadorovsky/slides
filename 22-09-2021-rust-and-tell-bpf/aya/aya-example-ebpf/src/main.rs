#![no_std]
#![no_main]

use aya_bpf::{
    helpers,
    macros::{kprobe, map},
    maps::HashMap,
    programs::ProbeContext,
    BpfContext,
};
use aya_log_ebpf::info;

#[map(name = "processes")]
static mut PROCESSES: HashMap<u32, u32> = HashMap::<u32, u32>::with_max_entries(4194304, 0);

#[kprobe(name = "kernel_clone")]
pub fn kernel_clone(ctx: ProbeContext) -> u32 {
    match unsafe { try_kernel_clone(ctx) } {
        Ok(ret) => ret,
        Err(ret) => ret,
    }
}

unsafe fn try_kernel_clone(ctx: ProbeContext) -> Result<u32, u32> {
    // let comm = match helpers::bpf_get_current_comm() {
    //     Ok(comm) => comm,
    //     Err(_) => return Ok(0),
    // };
    // info!(&ctx, "pid of forking process: {}", ctx.pid());
    let pid = ctx.pid();
    info!(&ctx, "some process forked");
    match PROCESSES.insert(&pid, &0, 0) {
        Ok(_) => return Ok(0),
        Err(_) => return Ok(0),
    };

    Ok(0)
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    unreachable!()
}
