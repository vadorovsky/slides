---
title:
- Developing and managing eBPF programs with Rust
author:
- Michal Rostecki (@vadorovsky)
theme:
- Copenhagen
date:
- September 22, 2021
---
# About me

* he/him or they/them
* vadorovsky on Github, Gitlab, Discord, IRC, Matrix
* Software Engineer @ SUSE
* background: Python, OpenStack, Golang, k8s
* nowadays: Rust, Go, C, k8s, eBPF, Linux kernel

# What is eBPF?

**eBPF** is a mechanism in Linux kernel which allows to inject programs
triggered by various integration points like:

* network I/O
  * raw network packet (XDP)
  * `sk_buff`
* application sockets
* kernel or userspace function calls (tracepoints)
* LSM hooks
* and more...

Those programs can be written in BPF "assembly", C or (recently) Rust.
Compilation to BPF bytecode is done by LLVM.

# Tracing or making decisions

* Some BPF programs just trace events/functions.
* Some BPF programs can make decisions what happens:
  * tc, XDP programs can decide what happens with the packet
  * LSM programs can decide whether an event is allowed to happen

# Programs and loader

BPF programs can be loaded by `bpf()` system call.

Usually projects using BPF provide also an userspace binary which loads BPF
programs with `bpf()`.

# BPF maps

**BPF maps** are data structures in which BPF programs can store data and share
it with userspace. Userspace can have an access to BPF maps through virtual
`/sys/fs/bpf` filesystem.

# Example of BPG program (in C)

* [BPF program](https://github.com/vadorovsky/slides/blob/main/22-09-2021-rust-and-tell-bpf/c/fork_trace.bpf.c)
* [userspace program](https://github.com/vadorovsky/slides/blob/main/22-09-2021-rust-and-tell-bpf/c/fork_trace.c)

# Rust support

There are two ways to use BPF in Rust:

* [libbpf-rs](https://github.com/libbpf/libbpf-rs) - stable library with full
  coverage of BPF features, but it's only for managing BPF programs (which
  still have to be written in C) and it wraps libbpf (written in C)
* [aya](https://github.com/aya-rs/aya) - experimental library, not covering all
  BPF features and program types, but allows to write BPF programs in Rust, C
  is not used at all

# libbpf-rs example

* [BPF program](https://github.com/vadorovsky/slides/blob/main/22-09-2021-rust-and-tell-bpf/libbpf-rs/src/bpf/fork_trace.bpf.c) (in C)
* [userspace program](https://github.com/vadorovsky/slides/blob/main/22-09-2021-rust-and-tell-bpf/libbpf-rs/src/main.rs) (in Rust)

# aya example

* [BPF program](https://github.com/vadorovsky/slides/blob/main/22-09-2021-rust-and-tell-bpf/aya/aya-example-ebpf/src/main.rs)
* [userspace program](https://github.com/vadorovsky/slides/blob/main/22-09-2021-rust-and-tell-bpf/aya/aya-example/src/main.rs)

# lockc

Website: [https://rancher-sandbox.github.io/lockc/](https://rancher-sandbox.github.io/lockc/)

Github: [https://github.com/rancher-sandbox/lockc](https://github.com/rancher-sandbox/lockc)

Experimental project trying to use **eBPF LSM** to harden containers.

# Containers do not contain

Containers are not as isolated as VMs and they expose a lot of information from host.

* Some filesystems are not namespaced - many subdirs of `/sys` and `/proc` have
  the same content on host and inside container.
* Docker (by default) allows everyone in `docker` group to mount everything
  inside container - including `/` or `docker.sock`.
* `chroot` is possible inside containers.
* etc.

**lockc** aims to limit those possibilities for regular users.

# How lockc uses BPF?

* For now it uses **libbpf-rs** and contains code both in C and Rust.
* **aya** lacks the support for LSM programs.
* As soon as aya supports LSM, we will be more than happy to have everything in
  Rust!

# Any questions?
