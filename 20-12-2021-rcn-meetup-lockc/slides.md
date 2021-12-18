---
title:
- lockc - container security with eBPF LSM
author:
- Michal Rostecki (@vadorovsky)
theme:
- Copenhagen
date:
- December 20, 2021
---
# About me

* Software Engineer @ SUSE
* vadorovsky on Github, Gitlab, Discord, IRC, Matrix
* background: Python, OpenStack, Golang, k8s
* nowadays: Rust, Go, C, k8s, eBPF

# First question

Do you think that containers are secure?

# Containers do not contain

Many people assume that containers

* provide the same or similar isolation to virtual machines
* protect the host system
* sandbox applications

All the points (except the first one) are partially true, but not entirely.

# What's problematic about container security?

Containers come with their own rootfs, but some parts of the filesystem are
still **the same as on the host system**. That includes:

* kernel fileystems under */sys*
* sysctls under */proc/sys*
  * some of them are namespaced, some are not

# What else is problematic?

Many container engines allow users to bind mount **everything** from the host
system. That includes:

* host rootfs
* Docker socket

Mounting those might give users in fact **passwordless sudo**.

# Breaking out from the container #1

```bash
$ docker run --rm -it -v /:/rootfs \
    opensuse/tumbleweed:latest bash
abb67212044d:/ # chroot /rootfs
sh-4.4#
```

# Breaking out from the container #2

```bash
$ docker run --rm -it -v \
    /var/run/docker.sock:/var/run/docker.sock \
    docker sh
/ # docker ps
CONTAINER ID   IMAGE     COMMAND                  CREATED         STATUS         PORTS     NAMES
066811b60d69   docker    "docker-entrypoint.s…"   5 seconds ago   Up 5 seconds             suspicious_liskov
/ # docker run --rm --privileged -it -v \
    /:/rootfs opensuse/tumbleweed:latest bash
54b08e30fd9e:/ # chroot /rootfs
sh-4.4#
```

# Why do we even allow that?

* Before container era, there was a hard distinction between **infrastructure**
  (libvirt, OVS etc.) and **workloads** (actual VMs). They would never get
  mixed. Intrastructure was always deployed by external configuration
  management tool (Ansible, Chef, Puppet).
* During container era, difference between **infrastructure** and **workloads**
  is not so obvious. We run parts of Kubernetes, CNI plugins, service meshes
  as containers / pods. That's convenient, but that's also a challenge in terms
  of security.

# Kubernetes cares about security

Kubernetes community is doing a great job about security and to prevent
malicious behavior we mentioned.

* PodSecurity admission controller
  * not allowing host mounts
* kubewarden, Open Policy Agent
* core Kubernetes features
  * seccomp
  * hiding unsafe / not namespaced sysctls
* sigstore
  * signing and verifying images

But they all protect on the **cluster level**. None of those mechanisms works
on the node, close to the container runtime and the kernel.

# PodSecurity admission controller

The official k8s addmission controller comes with **3 policy levels**:

* baseline
* restricted
* privileged

And with **3 modes of operation**:

* enforce
* audit
* warn

# Applying PS admission controller policies

Those policies and modes are applied by adding labels to **namespaces** (in
Kubernetes). Those labels are:

* `pod-security.kubernetes.io/enforce`
* `pod-security.kubernetes.io/enforce-version`
* `pod-security.kubernetes.io/audit`
* `pod-security.kubernetes.io/audit-version`
* `pod-security.kubernetes.io/warn`
* `pod-security.kubernetes-io/warn-version`

# Example #1

```
apiVersion: v1
kind: Namespace
metadata:
  name: baseline
  labels:
    pod-security.kubernetes.io/enforce: baseline
    pod-security.kubernetes.io/enforce-version: v1.23
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/audit-version: v1.23
    pod-security.kubernetes.io/warn: restricted
    pod-security.kubernetes-io/warn-version: v1.23
```

# Example #2

```
apiVersion: v1
kind: Namespace
metadata:
  name: privileged
  labels:
    pod-security.kubernetes.io/enforce: privileged
    pod-security.kubernetes.io/enforce-version: v1.23
```

# lockc

**[lockc](https://rancher-sandbox.github.io/lockc/)** is the project which
aims to bring security policies to nodes and container runtimes. Those policies
are enforced on the kernel with **eBPF**.

# eBPF

**[eBPF](https://ebpf.io/)** is a mechanism in Linux kernel which allows to
inject programs triggered by the kernel with various integration points like:

* network I/O
  * raw network packet (XDP)
  * `sk_buff`
* application sockets
* kernel or userspace function calls (tracepoints)
* Linux Security Module hooks
* and more...

# BPF maps

**BPF maps** are data structures stored in memory which are used by BPF
programs and can be shared to userspace via **bpffs**.

There are many types of maps. The most popular ones are:

* `BPF_MAP_TYPE_HASH`
* `BPF_MAP_TYPE_ARRAY`

# BPF LSM

Since kernel 5.7 it's possible to write **eBPF programs** which attach to
**[LSM hooks](https://www.kernel.org/doc/html/latest/bpf/bpf_lsm.html)**. The
same hooks which security modules like SELinux or AppArmor use. That allows to
write LSM-based MAC systems without changes to upstream kernel.

# libbpf-rs

**[libbpf-rs](https://github.com/libbpf/libbpf-rs)** is a Rust library which
allows to manage BPF entities from userspace:

* loading programs
* managing the content of maps

But BPF programs have to be written in **restricted C**.

**lockc** currently uses **libbpf-rs**.

# aya

**[aya](https://aya-rs.github.io/book/)** is a Rust library which allows to
write both BPF programs and userspace part in Rust. It's fresh and still
experimental.

Advantages of aya:

* no LLVM needed, Rust and cargo are enough
* focused in being easy for developers
* **aya-log** library which handles logging with automatically created perf
   event maps
* BPF programs can be shared on crates.io

**lockc** might switch to **aya** once it gets support for LSM programs (there
is already a [draft PR opened](https://github.com/aya-rs/aya/pull/68)).

# Architecture of lockc

**lockc** consists of the single daemon performing two tasks:

* loading and attaching eBPF programs which
  * tracking new processes
  * enforcinng policies of them, if they are cotainerized
* observing **runc** processes
  * 

* **lockc-runc-wrapper** - wrapper for **runc** which registers new containers
  and the runc process as tracked containerized process
* **BPF programs** 
  * program for tracking new containers - all children of the registered
    **runc** processes
  * programs attaching to **LSM hooks** and enforcing policies on processes
    registered as containerized
* **lockcd** - daemon which loads and manages BPF programs

# lockc policies

* For now **lockc** supports the same policy levels as PodSecurity admission
  controller in k8s:
  * baseline
  * restricted
  * privileged (with Docker - able to be chosen only by root, no plans to have
    it for rootless containers and containers started by users)
* In far future we might consider allowing to write custom policies
  * with YAML configuration
  * with Rust code (once we switch to aya)

# What lockc protects against?

So far we focused on filesystem access:

* **bind mounts** - lockc disallows **all** bind mounts for restricted containers,
  disallows the most of bind mounts for baseline containers
* **directory access** - there is a list of directory prefixes which restricted
  and baseline containers are allowed to access
* **syslog** - containers are not allowed to look at the kernel syslog

# Choosing policies

* **Kubernetes:** the same way as choosing PodSecurity admission controller
  policies
* **Docker:** `docker run --label org.lockc.policy=[level]`

# Remarks

* the project is still experimental
* not meant for Kubernetes clusters you care about
* API might change

# Quickstart (Kubernetes)

[https://rancher-sandbox.github.io/lockc-helm-charts/](https://rancher-sandbox.github.io/lockc-helm-charts/)

```bash
$ helm repo add lockc https://rancher-sandbox.github.io/lockc-helm-charts/
$ helm install --create-namespace -n lockc lockc lockc/lockc
```

# Future plans (security)

For baseline and restricted containers:

* allow to execute only specific binaries (CMD/ENTRYPOINT + few essentials)
* make the most of directories inside containers read-only and guarantee
  immutability of binaries
* allow to run only as a regular user
* restrict network and socket communication
* disallow to access kernel keyring, set system time
* disallow to change namespace inside container

# bpfcontain-rs

**[bpfcontain-rs](https://github.com/willfindlay/bpfcontain-rs)** is a project
very similar to lockc - also using **BPF LSM** for hardening containers.

We are alredy in touch with the author and plan to merge projects step by step.

# landlock

**landlock** is the new LSM available in kernel 5.13. It allows to create
security rules by unpriviledged users for new processes.

So far it handles filesystem access.

It might be seen as a good fit for containers.

# Why landlock wasn't chosen for lockc?

* It didn't end up using BPF (because of not requiring privileges).
* It requires changes in upstream kernel to cover more LSM hooks.
* landlock developers seem to push for natively supporting it by projects like
  [runc](https://github.com/opencontainers/runc/pull/3194),
  [containerd](https://github.com/containerd/containerd/pull/6120) or
  [tar](https://lists.gnu.org/archive/html/bug-tar/2021-04/msg00002.html).
  Isn't it too much effort?
* We would like to not require changes in any of upstream projects, if possible.

# Demo

* Mount policies
* Filesystem policies (allow/deny-listing of directories)
