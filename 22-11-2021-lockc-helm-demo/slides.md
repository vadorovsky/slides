---
title:
- lockc - installation with Helm, fanotify, uprobes
author:
- Michal Rostecki
theme:
- Copenhagen
date:
- October 21, 2021
---
# What changed in lockc since last demo?

We made many things simplier

* installation with helm does everything
* we don't wrap runc anymore
* we don't require any containerd configuration changes

# How did we remove the wrapper?

* Previously, we were wrapping runc to get all runc commands and create
  policies before container get created.
* But `fanotify(7)` allows to monitor and freeze processes just with a syscall.
* So now, we just have a deamon which watches runc processes.

# We also moved container registration to BPF programs

* *uprobes* are BPF programs which hook into userspace program functions. They
  get triggered in the kernel every time our specific userspace function is
  called.
* We created `add_container`, `delete_container`, `add_process` functions which
  exist only to trigger BPF programs and pass container metadata (ID, policy)
  to them.
* BPF programs, based on that information, save info in BPF maps, for the other
  BPF programs which do policy enforcemeent.

# To sum it up

What **lockcd** does when a container is created:

* it gets a notification about new runc process
* parses its arguments, gets the ID and `config.json` of the container
* based on `config.json`, it figures out whether container comes from engines
  supported by us (CRI containerd, Docker)
* determines the policy
* triggers *uprobe* BPF program which registers a new container

# lockc container image

lockc container image, designed to be used with Kubernetes, has two binaries:

* **lockcd** - daemon which monitors runc, loads and manages BPF programs
* **bpftool** - tool for debugging BPF programs, to be used only with `kubectl
  exec`

# Demo

# Plans for next weeks

* Release lockc, then start advertising that it can be tried out with Helm.
* Migrate from libbpf-rs to aya-rs. Which means writing both BPF programs and
  userspace part of lockc in Rust.
* Block access to the ServiceAccount token for regular containers.
  * Yes, it's available in every pod in `/var/run/secrets/kubernetes.io`.
* Enforce running regular containers with non-root user.

# Why Aya?

It's not just about "rewriting everything to Rust" as a Rust fanboy (although
partially it is ;) ).

But there are also other reasons and advantages or aya over libbpf-rs:

* no dependency on LLVM - it can use internal LLVM of Rust compiler
* no need to convert C structures (used by BPF programs) to Rust structures to
  use them in both places (with FFI/bindgen)
* there is aya-log library which implements logging with perf event buffer with
  almost no boilerplate code; with libbpf-rs you need to implement such logging
  from scratch
