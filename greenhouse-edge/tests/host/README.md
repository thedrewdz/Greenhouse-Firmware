# Host unit tests (native)

Host-runnable test harness that compiles and executes the **real firmware code**
from [`src/`](../../src) natively — no ESP-IDF toolchain or hardware required.
It is the WI-1 safety net for the standards-remediation work
(`.agent-output/tasks/edge-standards-remediation.md`).

## What it covers

### Pure-module tests (codec + backoff)

| Suite | Firmware under test | Notes |
|---|---|---|
| `test_codec_provisioning.c` | `gh_codec_parse_provisioning_payload` | Positive cases + table-driven negatives for every canonical `200x` status code; oversized/missing-field rejection. |
| `test_codec_serialization.c` | `gh_codec_build_provisioning_status_payload`, `gh_codec_build_response_payload`, `gh_codec_build_heartbeat_payload`, `gh_codec_parse_command_payload`, `gh_codec_parse_command_topic` | Built JSON is parsed back with cJSON so assertions are field-order independent. |
| `test_retry_backoff.c` | `gh_retry_backoff_*` (`src/retry_backoff.c`) | Canonical 1000/2000/4000/8000 schedule, ceiling saturation, 20% symmetric jitter bounds. Shared by the WiFi and MQTT connect state machines. |

### Fake-service tests (real service state machines vs. a fake ESP-IDF)

These compile the **real** service `.c` files and drive them against the fake
ESP-IDF layer in `fakes/` (controllable clock, event dispatch, WiFi/MQTT/NVS).

| Suite | Firmware under test | Issue | Notes |
|---|---|---|---|
| `test_service_mqtt.c` | `src/services_mqtt.c` | #14, #15 | MQTT connect-attempt timeout → reconnect; first-heartbeat publish failure attributed to MQTT and consuming the bootstrap retry budget → `bootstrap_failed`. |
| `test_service_network.c` | `src/services_network.c` | #14 | WiFi got-IP → connected + RSSI; connect-attempt timeout → retry; budget exhaustion → `bootstrap_failed`. |
| `test_service_provisioning_nvs.c` | `src/services_provisioning_config.c` | #16 | A/B candidate-write-then-promote, slot alternation, and failure-injection proving a failed candidate write or failed promotion retains the previously active slot. |

All suites call the real functions directly — there are **no** source-text
assertions and **no** reimplementations of firmware logic (which is what the
removed `tests/test_onboarding_contract.py` did).

## Running

```powershell
pwsh -File tests/host/run_tests.ps1
```

The script locates a Visual Studio C/C++ toolchain (`cl.exe`) via `vswhere`
(with a fallback scan of the standard install roots), imports the MSVC
environment, compiles the firmware modules + vendored deps + test sources into
`build/host/gh_host_tests.exe`, runs it, and propagates the Unity exit code
(non-zero on any failure — suitable for CI).

Only MSVC is wired up today because that is the toolchain present on the
development machine; the sources are plain C and portable to gcc/clang if a
CMake host target is added later.

## Layout

```
tests/host/
  run_tests.ps1      build + run driver (MSVC)
  shims/esp_err.h    minimal host stand-in so firmware public headers compile
  fakes/             fake ESP-IDF surface (clock, event loop, wifi, mqtt, nvs)
                     + test-control hooks (fakes.h) so real service .c files run
  vendor/cjson/      cJSON v1.7.18 (matches the ESP-IDF `json` component API)
  vendor/unity/      Unity v2.6.0 (ThrowTheSwitch) test framework
  src/               the host test suites + Unity entry point
```

`vendor/` holds pinned third-party sources and is committed so the build is
self-contained and reproducible offline.

## Not covered here (tracked separately)

- **#17** fresh-device BLE onboarding through first `gh/heartbeat` — a
  hardware-in-loop smoke test, not a host test. The procedure/checklist lives at
  [`tests/hil/README.md`](../hil/README.md); it must be run on real hardware.
- The BLE **GATT write-size rejection** portion of #13 (NimBLE `on_gatt_access`)
  is exercised by the HIL smoke, not the host build.
