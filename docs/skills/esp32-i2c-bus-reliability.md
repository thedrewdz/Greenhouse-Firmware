# Skill: ESP32 I2C Bus Reliability

## Purpose

Help coding agents implement robust I2C communication for ESP32-based Edge Units with multiple slot modules.

## Use This Skill When

- Building I2C discovery and slot probing logic.
- Adding sensor or actuator module drivers on shared I2C buses.
- Debugging intermittent bus faults and address conflicts.

## Bus Design Rules

- Keep one canonical I2C manager responsible for bus init, scan, and transaction policies.
- Represent each discovered module by address plus capability metadata.
- Validate address ranges against project conventions before accepting modules.
- Never hardcode sensor logic directly in the I2C manager.

## Transaction Rules

- Use bounded retry counts for read and write operations.
- Add per-transaction timeout handling and classify timeout separately from invalid data.
- Serialize access to each bus to avoid collisions.
- Mark module fault state after repeated failures and surface in heartbeat.

## Recovery Rules

- Detect stuck bus conditions and run bus recovery sequence when supported.
- If one module fails repeatedly, isolate it logically instead of stopping the full node.
- Keep successful modules operational even when one slot faults.

## Data and Error Normalization

- Normalize driver errors to stable project error codes.
- Keep slot-level fault state in memory and include it in heartbeat payload.
- Ensure read responses and heartbeat slot state stay consistent.

## Validation Checklist

- Cold boot scan correctly identifies all attached modules.
- Hot fault on one address does not crash main loop.
- Invalid or duplicate addresses are rejected with explicit error codes.
- Telemetry and fault reporting remain deterministic under retry conditions.
