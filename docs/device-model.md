# Device Model

# Overview

The greenhouse platform uses a distributed device model built around modular ESP32-based nodes.
Each Edge Unit hosts a fixed number of slots for sensor and actuator connections.
Attached sensor and actuator MPUs are responsible for local hardware translation and canonicalization.

This allows:
- flexibility
- future expansion
- dynamic discovery
- modular deployment

---

# Physical Edge Unit Bus Model

Each Edge Unit has a fixed slot count.

- Each slot uses a 4-wire cable.
- Connected sensor and actuator MPUs share the same I2C bus on the Edge Unit.
- A slot may be unassigned, sensor-assigned, or actuator-assigned.

## Sensor MPUs

Sensors use a small onboard MPU (for example Arduino Nano or Pro class boards).

- The sensor MPU owns sensor-specific hardware interactions.
- The sensor MPU publishes canonicalized values to the hosting Edge Unit.
- Sensor identity uses a unique I2C address.

Initial addressing convention:

- 0x20-0x2F: moisture sensor family
- Additional ranges should be reserved per sensor family as needed.

In rare cases where the attached sensing hardware also uses I2C, the sensor MPU may be ESP32-based to use dual I2C buses.

## Actuator MPUs

Actuator MPUs receive generic instructions from the hosting Edge Unit and translate them to actuator-specific operations.

- Execute actuator-specific behavior.
- Return canonicalized response status to the hosting Edge Unit.
- Keep actuator safety behavior local to the actuator module.

---

# Device Categories

## Sensor Node

A device primarily responsible for telemetry acquisition.

Examples:
- environmental sensor nodes
- water quality monitors
- climate monitoring stations
- soil monitoring probes

Primary responsibilities:
- read sensors
- normalize telemetry
- publish telemetry to MQTT
- report health/status

---

## Actuator Node

A device responsible for controlling physical equipment (through relays)

Examples:
- pumps
- fans
- lighting
- solenoid valves

Primary responsibilities:
- subscribe to MQTT commands
- execute operations
- report actuator state
- fail safely during faults

---

## Hybrid Node

A node that both:
- reads sensors
- controls actuators

Examples:
- irrigation controller with flow sensing
- hydroponic dosing controller
- nutrient management station

---

## Future Vision Node

Camera or machine-vision device. Most probably just streaming frames from a web cam to be processed online.

Potential capabilities:
- growth analysis
- disease detection
- pest identification
- canopy monitoring
- time-lapse imaging

These systems should remain logically separated from critical automation systems.

---

# Device Identity

Each device must contain:

- unique device ID - WiFi MAC address
- firmware version
- hardware revision
- capability list - analogous with connected edge modules
- general fault code - 0 means none
- fault list - fault states per capability or edge module

---

# Device Metadata

Example metadata structure:

```json
{
  "device_id": "1ADD5912AF61",
  "hardware_revision": "A",
  "firmware_version": "1.0.0",
  "uptime_seconds": 92384,
  "wifi_rssi": -61,
  "manufacturer": "GreenhousePlatform",
  "capabilities": [
    "temperature",
    "humidity",
    "light"
  ]
}
```

# Onboarding Model (Phase 1)

Phase 1 onboarding is BLE-first for user-friendly setup.

Rationale:

- no requirement for user-managed cables
- no requirement for user-managed static IP configuration
- better support for non-technical users

Baseline assumption:

- Main Control Unit hardware is Raspberry Pi 4B (BLE-capable)

Provisioning sequence:

- Edge Unit boots unprovisioned and enters BLE provisioning mode
- Main Control Unit discovers and pairs with the Edge Unit over BLE
- Main Control Unit sends WiFi credentials and bootstrap MQTT endpoint
- Edge Unit validates and stores provisioning data
- Edge Unit connects to WiFi and MQTT
- Edge Unit publishes first heartbeat
- Main Control Unit marks onboarding complete

Recovery path:

- Wired provisioning may be used as an optional fallback during support or manufacturing

# Device Registration

- Devices register themselves during startup
- Devices persist provisioning data required for network bootstrap.
- Runtime configuration is still sourced from the Main Control Unit.
- From the device perspective, registration and startup are unified after provisioning is complete.

Registration process:

- Boot
- If unprovisioned, perform BLE onboarding first
- Query connected slot modules and capture current slot state
- Connect to WiFi
- Connect to MQTT
- Publish heartbeat containing device metadata and connected slot state
- Receive configuration from main unit via MQTT
- Enter operational state

Main control unit behavior on heartbeat:

- If no configuration exists for the Edge Unit, treat it as a newly discovered unit and trigger setup/onboarding.
- If configuration exists but discovered slot topology differs from stored configuration, trigger configuration update flow.
- If configuration exists and topology matches, continue normal operation.

# Capability-Based Design

The platform should reason about capabilities and their location where possible rather than specific hardware implementations

Examples of capabilities:

- temperature
- humidity
- pump
- valve
- camera
- co2
- ph
- ec-tds
- light
- time of day
- season

Advantages:

- interchangeable hardware
- flexible deployments
- easier future expansion
- simpler automation logic

# Device Lifecycle
## Boot Phase

During startup a device:

- Boot
- Query connected slot modules and capture current slot state
- Connect to WiFi
- Connect to MQTT
- Publish heartbeat containing device metadata and connected slot state
- Receive configuration from main unit via MQTT
- Enter operational state

## Operational Phase

During normal operation devices:

- receive commands
- publish telemetry
- report heartbeat
- monitor local faults
- [future] support OTA updates

## Fault Phase

If failures occur:

- errors are reported
- safe-state logic activates
- watchdog recovery may occur
- reconnect attempts continue

# Offline Recovery

Devices should tolerate:

- temporary WiFi outages
- MQTT outages
- controller restarts
- power interruptions
- [future] temporary loss of the main unit

without requiring manual intervention.

# Heartbeat Model

Devices publish periodic heartbeat messages.

Heartbeat interval:

- typically 30–60 seconds

Heartbeat payload should include:

- Device metadata
- Discovered slot topology
- Current canonicalized state for connected sensors and actuators
- Current fault states (general and per slot when available)

This heartbeat model is used for both first-time discovery and subsequent restarts.

# Firmware Architecture

ESP32 firmware should remain modular.

## Proposed Firmware Stack (Phase 1)

- Framework: ESP-IDF
- MQTT client: `esp-mqtt` (Espressif managed)
- JSON codec library: `cJSON`
- RTOS model: FreeRTOS tasks and queues/event groups
- Transport: WiFi station mode for edge connectivity

POC experiments used `knolleary/PubSubClient` successfully for connectivity validation. For this project, the standard implementation should use `esp-mqtt` for production firmware behavior and long-term maintainability.

## Message Codec Boundary

To keep transport and payload concerns decoupled, firmware should implement a clear codec boundary:

- Transport layer (`esp-mqtt`) handles topic subscription, publish, reconnect, and QoS behavior.
- Codec layer (`cJSON` in Phase 1) handles payload parse, validation, and serialization.
- Application logic consumes canonical internal message models and should not parse raw JSON directly.

This boundary allows payload-format changes later without rewriting transport or business logic.

## Phase 2 Payload Format Experiment

Phase 1 standard remains JSON using `cJSON`.

In Phase 2, we may run a controlled experiment to compare JSON with a compact binary format (for example CBOR or Protobuf) if measured constraints justify a change.

Decision to pivot should be based on observed limits in:

- memory pressure
- CPU parsing cost
- bandwidth usage at real heartbeat and command rates
- operational reliability under reconnect and burst conditions

Recommended layering:

```
Application Layer
    |
Service Layer
    |
Hardware Abstraction Layer
    |
Drivers
    |
Hardware
```

# Firmware Responsibilities
## Application Layer

Responsible for:

- telemetry generation
- actuator activation/deactivation
- actuator safeguards by type
- command routing

## Service Layer

Responsible for:

- MQTT (`esp-mqtt`)
- WiFi
- heartbeat
- [future] OTA updates

## Hardware Abstraction Layer

Responsible for:

- abstracting physical hardware
- standardizing interfaces
- isolating drivers

## Drivers

Responsible for:

- sensors
- ADC
- relays
- PWM
- I2C
- SPI
- UART

# Configuration Model

Devices should support:

- remote configuration updates from the Main Control Unit
- runtime application of received configuration

In Phase 1, Edge Units do not persist long-term configuration. The Main Control Unit is the source of truth for configuration and sends configuration to edge units.

The Main Control Unit is also the custodian of configuration lifecycle decisions.

- New edge unit discovered: prompt onboarding flow.
- Existing edge unit changed: prompt reconfiguration flow.
- Existing edge unit unchanged: apply stored configuration without user intervention.

# Future OTA Update Model

All production devices should support OTA firmware updates.

OTA requirements:

- integrity validation
- rollback support
- staged deployment
- version tracking

# Safety Model

Actuator devices must define:

- safe startup state
- safe failure state
- watchdog behavior

Examples:

- pumps default OFF
- heaters default OFF
- valves default CLOSED

Critical systems should fail safely during:

- reboot
- MQTT loss
- software crash
- watchdog reset

# Local Autonomy

Future devices may support limited local autonomy.

Example:

- local emergency thermal shutdown
- local reservoir overflow protection
- local pump timeout logic
- local scheduling if the central control unit becomes unavailable

These protections operate independently of the central controller.

# Time Synchronization

To avoid over-developing, we will not attempt to maintain time on all devices.

- Central control unit (main unit) is responsible for timing
- Edge Units maintain an integer message index that is incremented with every payload published
- message index can be assigned during startup routine based on stored value from main unit.

