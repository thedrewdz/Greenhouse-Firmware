# Greenhouse Peripheral Firmware Context

This context defines the shared domain language for ESP32 peripheral firmware in the Greenhouse platform.
Use these terms consistently across code, docs, and design discussions.

## Language

### Platform Topology

**Control Unit**:
The local system authority that runs automation, coordinates nodes, and issues commands over MQTT.
_Avoid_: Server, cloud backend

**Peripheral Node**:
An ESP32 firmware instance connected to the control unit over MQTT.
_Avoid_: Device (generic), endpoint

**Sensor Node**:
A peripheral node role that reports measurements from attached sensors.
_Avoid_: Reader node

**Actuator Node**:
A peripheral node role that executes hardware actions through attached actuators.
_Avoid_: Output node

### Hardware Model

**Slot**:
A numbered attachment point on a peripheral node for one module.
_Avoid_: Port (ambiguous), channel

**Module**:
A sensor or actuator unit attached to a slot.
_Avoid_: Peripheral (ambiguous), plugin

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
