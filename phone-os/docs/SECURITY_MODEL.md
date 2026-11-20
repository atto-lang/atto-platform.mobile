# Oreslang Phone app security domains — prototype contract

**Evidence boundary:** this PR validates trusted policy records on a host. It does **not** install Linux restrictions, launch untrusted apps, compile Oreslang actors, or prove mobile device isolation.

## Isolation decision

* Independently installed, mutually untrusted apps require distinct OS process address spaces and effective security credentials.
* Each app is an Oreslang supervisor/actor group containing many actor-private arenas.
* Actor borrow checking, sealed state, and GraalVM internal isolates are useful **inside** an app, but cannot be treated as kernel-enforced isolation.
* GraalVM external-process isolates may implement a process boundary only when the OS launcher independently applies the same credentials, permissions, and resource restrictions.
* This phone-OS prototype is for native Linux AArch64. Existing Android JNI and iOS XCFramework paths are separately sandboxed app integration targets. iOS does not provide a generic arbitrary app-child-process interface.

## Threats and trust

Untrusted app code controls its requested capabilities, calls, guest data, threads, and messages. The kernel, signed package verifier, privileged broker, and installed mandatory-access-control policy are the initial trust base. Kernel compromise and a malicious privileged broker are out of scope for this first milestone.

Capabilities **requested by apps** are not permissions. Grants and UIDs are assigned by trusted OS policy, after verifying application identity and signature. Never deserialize app-controlled values directly into the trusted launch policy structure.

## Host admission (implemented)

The dependency-free C API accepts a **trusted policy record** and rejects:
* malformed dotted lowercase app identifiers;
* prototype-reserved UIDs outside 100000–999999;
* paths outside the canonical per-app executable/data locations;
* memory limits outside 16 MiB–4 GiB or invalid CPU periods/quotas;
* unknown capability bits and requests exceeding trusted grants;
* duplicate app IDs, UIDs, executable locations, or data roots among up to 1024 entries.

Paths are only **lexically** checked. This API does not resolve symlinks, check mount identities, authenticate files, enforce credentials, or confer sandbox security. Its prototype numeric ranges are not a deployed allocation policy.

Canonical locations:
* /opt/ores-phone/apps/{app_id}/bin/app
* /var/lib/ores-phone/apps/{app_id}

## Fail-closed launcher sequence (not implemented)

1. Verify signed artifact/manifest and allocate a unique trusted app identity/UID and a security domain label.
2. Open executable and private root relative to pinned trusted directory handles; reject symlinks, magic links, traversal, unsafe ownership, and rename/mount substitution. Execute the verified descriptor, not a subsequently reopened string path.
3. Install immutable/read-only runtime mappings, private writable data views, mount/PID/network namespaces where required, and bounded cgroup-v2 CPU/memory/PIDs/IO controls.
4. Drop supplementary groups, UIDs/GIDs, ambient and bounding capabilities, and all unintended descriptors. Set no_new_privs. Install a seccomp policy, LSM confinement, and restricted devices/network brokers.
5. Have the trusted parent **verify** the child's final credentials, security label, namespaces, cgroup placement, FD set, and installed restrictions. Refuse to exec if any mandatory step fails; never silently run unsandboxed.
6. Supervise, revoke handles, terminate process groups safely, and clean up all state. Shared-memory IPC requires explicit capability ownership and revocation.

No static path check, manifest validation, or user-space-only actor isolation establishes a security boundary. Namespace identity alone does not prove separate UIDs or inability to ptrace other processes.

## Evidence gates

| Gate | Status |
| --- | --- |
| Host policy positive/negative cases, GCC/Clang | Local tests executed; review exact Git blobs |
| Host ASan/UBSan | Local tests executed |
| Privileged sandbox launcher and OS permission checks | NOT IMPLEMENTED |
| Kernel-denied cross-app file/memory access | NOT TESTED |
| IPC capability forging and lifecycle | NOT TESTED |
| Resource exhaustion / crash containment | NOT TESTED |
| Actual compiled .ores apps on Linux AArch64 | NOT TESTED |
| Android and iOS physical device isolation | NOT TESTED |

Next PR must implement an independently reviewed **privileged launcher** and real kernel-boundary tests. A runner that lacks required privileges must fail the gate rather than skip and claim success. Compiler integration with real Oreslang actors is a subsequent, independently evidenced milestone.
