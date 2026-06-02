# Greenhouse Edge Firmware Context

This context defines the shared domain language for ESP32 edge firmware in the Greenhouse platform.
Use these terms consistently across code, docs, and design discussions.

## Language

### Platform Topology

**Control Unit**:
The local system authority that runs automation, coordinates nodes, and issues commands over MQTT.
_Avoid_: Server, cloud backend

**Edge Unit**:
An ESP32 firmware instance connected to the control unit over MQTT.
_Avoid_: Device (generic), endpoint

**Sensor Node**:
An Edge Unit role that reports measurements from attached sensors.
_Avoid_: Reader node

**Actuator Node**:
An Edge Unit role that executes hardware actions through attached actuators.
_Avoid_: Output node

### Hardware Model

**Slot**:
A numbered attachment point on an Edge Unit for one module.
_Avoid_: Port (ambiguous), channel

**Module**:
A sensor or actuator unit attached to a slot.
_Avoid_: Addon (ambiguous), plugin

**Slot Fault Isolation**:
A fault containment rule where one failing slot does not cascade failure to other slots.
_Avoid_: Global I2C reset strategy

### Messaging

**Telemetry**:
Published measurement or state data emitted by a node.
_Avoid_: Metrics (infra meaning), logs

**Heartbeat**:
A periodic liveness and status message from a node.
_Avoid_: Ping packet

**Command Topic**:
The MQTT topic namespace used by the control unit to request node actions.
_Avoid_: RPC endpoint

### Provisioning and Onboarding

**Onboarding**:
The first-time setup flow where a new Edge Unit receives bootstrap connectivity data from the Main Control Unit.
_Avoid_: Manual preconfiguration

**Provisioning Mode**:
Temporary setup state where an unprovisioned Edge Unit advertises over BLE and accepts onboarding data.
_Avoid_: Permanent pairing mode

**BLE Provisioning**:
The Phase 1 onboarding transport used to exchange WiFi credentials and bootstrap MQTT endpoint data.
_Avoid_: Static-IP-only bootstrap

**Canonical Payload**:
The project-standard message shape and field semantics for MQTT payloads.
_Avoid_: Ad hoc payload

### Reliability and Safety

**Offline Buffering**:
Bounded local retention of outbound messages while connectivity is unavailable.
_Avoid_: Infinite queueing

**Failsafe State**:
The safe actuator condition maintained during uncertain or degraded conditions.
_Avoid_: Last-known-good (if unsafe)
