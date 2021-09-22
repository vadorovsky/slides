#include <signal.h>
#include <unistd.h>
#include "fork_trace.skel.h"

static volatile sig_atomic_t stop;

/* Signal handler */
void sig_int(int signo)
{
	stop = 1;
}

int main(int argc, char **argv)
{
	struct fork_trace_bpf *skel;
	int err;

	/* Bump RLIMIT_MEMLOCK to allow BPF sub-system to do anything */
	// bump_memlock_rlimit();

	/* Open, load and verify BPF application */
	skel = fork_trace_bpf__open_and_load();
	if (!skel) {
		fprintf(stderr, "Failed to open BPF skeleton\n");
		return 1;
	}

	/* Attach tracepoint handler */
	err = fork_trace_bpf__attach(skel);
	if (err) {
		fprintf(stderr, "Failed to attach BPF skeleton\n");
		goto cleanup;
	}

	printf("Successfully started! Please run `sudo bpftool prog trace` "
	       "to see output of BPF programs.\n");

	while (!stop) {
		fprintf(stderr, ".");
		sleep(1);
	}

cleanup:
	fork_trace_bpf__destroy(skel);
	return -err;
}
