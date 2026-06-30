# Skill: Embedded Resource Budgets

## Purpose

Guide coding agents to keep Edge Unit firmware within the MCU's resource limits and to avoid complexity the hardware cannot sustain. This skill makes RAM, flash, stack, and heap budgets — and the cost of abstraction — an explicit review gate, so clean design does not turn into bloat.

It complements the design skills: where `embedded-oo-coding-standards.md` and `esp32-firmware-architecture.md` argue for clean structure, this skill governs the trade-off when that structure costs memory, flash, or determinism the part can't spare.

## Use This Skill When

- Adding modules, layers, libraries, or abstractions to the firmware.
- Reviewing a change for memory, flash, or complexity impact.
- Deciding between static and dynamic allocation, or sizing FreeRTOS tasks.
- Judging whether an abstraction is worth its cost, or whether a design is over-engineered.

## Do Not Use This Skill When

- The task is pure documentation or contract work with no code impact.

## Budget Awareness Rules

- Know the target's budget before designing: SRAM, flash, per-task stack, and required heap headroom for the ESP32 variant in use.
- Read flash and RAM usage from the build output as part of every change. Treat shrinking margin as a review signal, not an afterthought.
- Keep a known safety margin; do not design to 100% of any resource.

## Allocation Rules

- Prefer static or pooled allocation. Avoid heap allocation after initialization, and never allocate in hot paths or ISRs.
- Bound every buffer and queue; size them deliberately, not generously "to be safe."
- Avoid heap fragmentation: no repeated allocate/free cycles in steady state, and no growing containers (for example `std::string`/`std::vector` expansion) in hot paths.
- Size FreeRTOS task stacks deliberately and verify with high-water-mark checks. Create one task per concern, but do not spawn tasks gratuitously — each carries a stack and scheduling cost.

## Abstraction Cost Rules

- Add an abstraction (layer, interface, virtual method, template) only when it removes real duplication or makes a concern testable. Each one has code-size and indirection cost; "cleaner in principle" is not sufficient justification.
- Prefer the simplest design that meets the contract. Reject speculative generality, plugin frameworks, and configuration systems the device does not need (see the static-registry guidance in `esp32-peripheral-registry-extensibility.md`).
- Be deliberate about C++ cost on embedded targets: virtual dispatch, templates that bloat flash, RTTI, exceptions, and iostreams all carry weight. Prefer fixed-point or integer math over floating point where the canonical contract allows and the part lacks an FPU.
- Keep logging bounded; verbose or per-loop logging costs flash and CPU and can mask timing problems.

## Measurement Rules

- Measure before optimizing and before pivoting a design for "performance." Base decisions on observed RAM, flash, CPU, and timing — not assumption.
- This is the basis for any future payload-format or architecture change: justify the pivot with measured memory pressure, parse cost, and bandwidth at real heartbeat and command rates (consistent with the canonical Phase 2 experiment guidance).

## Validation Checklist

- Flash and RAM margins after the change are known and acceptable.
- No new heap allocation in hot paths or ISRs; buffers and queues are bounded.
- Task stacks are sized and watermark-verified; no gratuitous tasks added.
- Each new abstraction is justified by real duplication removed or testability gained.
- No speculative generality or unused configurability introduced.
- Any performance or format claim is backed by measurement, not assumption.
