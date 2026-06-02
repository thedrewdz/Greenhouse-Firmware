# Skill: ESP-IDF Testing Strategy

## Purpose

Guide coding agents to build confidence in ESP32 firmware through layered testing: unit tests, host-side mock tests, and hardware-in-loop smoke validation.

## Use This Skill When

- Adding or refactoring firmware modules.
- Changing command, telemetry, or heartbeat contracts.
- Modifying I2C, WiFi, MQTT, or task coordination logic.

## Test Layers

### 1) Unit Tests (Fast)

Use ESP-IDF Unity tests for deterministic logic:
- payload validation
- state transitions
- retry and backoff policy math
- error mapping and normalization
- slot state merge and heartbeat projection

Rules:
- Keep hardware-independent logic isolated so it can be tested without device IO.
- Prefer table-driven tests for contract validation.
- Cover boundary cases and invalid payloads.

### 2) Host-Side Mock Tests (Integration-Lite)

Use host-based tests with fake or mock interfaces:
- fake I2C transport
- fake WiFi/MQTT service wrappers
- fake time providers

Rules:
- Verify orchestration behavior across module boundaries.
- Verify reconnect flow and resubscribe behavior.
- Verify one bad slot does not take down whole-node operation.

### 3) Hardware-In-Loop Smoke Tests (HIL)

Run on real ESP32 hardware for release confidence:
- boot and module discovery
- WiFi connect and reconnect
- MQTT publish and subscribe
- command ack paths
- heartbeat cadence and payload validity

Rules:
- Keep HIL suite small and deterministic.
- Record firmware version, board revision, and test timestamp.
- Fail release if core smoke tests fail.

## Coverage Priorities

- Command parse and validation for read and write topics.
- Error responses for invalid slot, invalid payload, and unsupported capability.
- I2C timeout and retry behavior.
- WiFi outage and broker restart recovery.
- Actuator fail-safe behavior during connectivity loss.

## Testability Design Rules

- Inject dependencies for transport, clock, and storage interfaces.
- Keep declarations in headers and implementations in source files.
- Avoid global singleton state where test isolation is required.
- Expose pure functions for payload and state transforms where practical.

## CI and Quality Gate

Minimum gate for firmware-affecting changes:
1. Unit test suite passes.
2. Host-mock integration suite passes.
3. HIL smoke checklist completed for release candidates.
4. Any schema change includes updated tests for both positive and negative cases.

## Output Checklist For Agents

- New or updated tests for changed behavior.
- Notes on what layer each new test belongs to.
- Explicit list of uncovered risks if hardware testing is unavailable.
