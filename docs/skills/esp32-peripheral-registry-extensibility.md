# Skill: ESP32 Peripheral Registry and Extensibility

## Purpose

Guide coding agents to implement the Edge Unit device-abstraction layer so that new peripheral (slot module) types are added cleanly, without editing central orchestration, command routing, or heartbeat logic.

This is the firmware expression of the platform's capability-based design: the Edge Unit reasons about capabilities (temperature, humidity, pump, valve, ...) and their slot location rather than specific hardware. The capability vocabulary, I2C address families, and slot model are owned by the Greenhouse Documentation repository; read that first, then apply these device-side extensibility patterns.

- Canonical entry point: https://github.com/thedrewdz/Greenhouse-Documentation/blob/main/README.md

## Use This Skill When

- Adding Edge Unit support for a new sensor or actuator peripheral type.
- Designing or refactoring how discovered slot modules are represented, constructed, and managed at runtime.
- Removing type-specific `if`/`switch` branches from the application layer.

## Do Not Use This Skill When

- The task is low-level I2C bus mechanics: scanning, retries, bus recovery (use `esp32-i2c-bus-reliability.md`).
- The task is the peripheral's own firmware (that lives in the Peripherals repository and its skill packs).
- The task is defining the capability vocabulary or address families themselves (those are canonical; raise gaps against the Greenhouse Documentation repository).

## Abstraction Rules

- Represent every slot module behind a small, common interface that the application layer uses polymorphically: identity/capability, read (sensor), actuate (actuator), and report fault/status. Sensors and actuators may use separate interfaces (interface segregation) rather than one broad interface.
- The application layer programs to these interfaces only. It must never branch on a concrete peripheral type or hardware model.
- Keep the interface behavior-based and deterministic; return explicit canonical results and stable status codes, not hidden side effects.

## Registry and Construction Rules

- Map a discovered capability (and/or validated address range) to a concrete driver through a single, data-driven registry/factory — one place in the codebase that knows how to construct a module for a discovered capability.
- Keep the registry a static table of `{capability/address-range -> factory}` entries. Adding a new peripheral type means adding its driver plus one registry entry — and nothing else. No edits to orchestration, telemetry collection, command routing, or heartbeat projection.
- Keep discovery and instantiation separate: the I2C layer yields address-plus-capability metadata; the registry turns that metadata into live module objects. Do not fuse scanning logic with construction logic.
- For an unknown or unsupported capability, construct an explicit "unsupported module" that reports a fault, rather than crashing or silently skipping. Surface it in the heartbeat so the Main Unit can see it.

## Runtime Management Rules

- Track each slot's lifecycle explicitly: discovered -> constructed -> initialized -> operational -> faulted (-> removed/absent). Keep this state per slot in memory.
- Command routing and telemetry collection iterate the registry of live modules polymorphically. There must be no per-type branches in the application layer.
- When a module faults repeatedly, isolate it logically (per `esp32-i2c-bus-reliability.md`) and reflect its state through the common interface; keep healthy modules operating.

## Avoid Over-Generalization

- A static registry table is the goal — not a runtime plugin framework, dynamic loader, or config-driven type system. Build only the extensibility the platform needs.
- Add an interface or indirection only when it removes real duplication across more than one peripheral type or makes a concern testable. See `embedded-resource-budgets.md` for the cost side of this trade-off.

## Validation Checklist

- Adding a new peripheral type touches only its driver and one registry entry; no central logic changes.
- The application layer contains no concrete-type branching over peripherals.
- Discovery metadata and module construction are separated.
- Unknown capabilities produce an explicit, fault-reporting module, not a crash or silent skip.
- Per-slot lifecycle state is tracked and consistently reflected in the heartbeat.
- Sensor and actuator interfaces are small and behavior-based.
