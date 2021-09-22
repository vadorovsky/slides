#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

/*
 * Max configurable PID limit (for x86_64, for the other architectures it's less
 * or equal).
 */
#define PID_MAX_LIMIT 4194304

#define TASK_COMM_LEN 16

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, PID_MAX_LIMIT);
	__type(key, pid_t);
	__type(value, u32);
} processes SEC(".maps");

SEC("fentry/kernel_clone")
int BPF_PROG(kernel_clone, struct kernel_clone_args *args)
{
	/* Get the parent pid */
	pid_t pid = bpf_get_current_pid_tgid() >> 32;

	/* Save the pid in BPF map */
	u32 val = 0;
	int err = bpf_map_update_elem(&processes, &pid, &val, 0);
	if (err < 0)
		return 0;

	/* Get the parent process name */
	char comm[TASK_COMM_LEN];
	bpf_get_current_comm(&comm, sizeof(comm));

	/* Log */
	bpf_printk("process %d (%s) is forking\n", pid, comm);

	return 0;
}

char LICENSE[] SEC("license") = "GPL";
