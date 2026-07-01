# Host unit tests (native)

Host-runnable test harness that compiles and executes the **real firmware code**
from [`src/`](../../src) natively — no ESP-IDF toolchain or hardware required.
It is the WI-1 safety net for the standards-remediation work
(`.agent-output/tasks/edge-standards-remediation.md`).

## What it covers

| Suite | Firmware under test | Notes |
|---|---|---|
| `test_codec_provisioning.c` | `gh_codec_parse_provisioning_payload` | Positive cases + table-driven negatives for every canonical `200x` status code; oversized/missing-field rejection. |
| `test_codec_serialization.c` | `gh_codec_build_provisioning_status_payload`, `gh_codec_build_response_payload`, `gh_codec_build_heartbeat_payload`, `gh_codec_parse_command_payload`, `gh_codec_parse_command_topic` | Built JSON is parsed back with cJSON so assertions are field-order independent. |
| `test_retry_backoff.c` | `gh_retry_backoff_*` (`src/retry_backoff.c`) | Canonical 1000/2000/4000/8000 schedule, ceiling saturation, 20% symmetric jitter bounds. Shared by the WiFi and MQTT connect state machines. |

These call the real functions directly — there are **no** source-text
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
  vendor/cjson/      cJSON v1.7.18 (matches the ESP-IDF `json` component API)
  vendor/unity/      Unity v2.6.0 (ThrowTheSwitch) test framework
  src/               the host test suites + Unity entry point
```

`vendor/` holds pinned third-party sources and is committed so the build is
self-contained and reproducible offline.

## Not covered here (tracked separately)

- **#14** fake-service WiFi/MQTT connect-timeout / reconnect ordering,
  **#15** first-heartbeat-failure bootstrap-budget accounting,
  **#16** NVS A/B failure-injection — these need the transport/clock/storage
  dependency-injection seams introduced by WI-2/WI-3; the backoff seam added
  here (`retry_backoff`) is the first of those.
- **#17** fresh-device BLE onboarding through first `gh/heartbeat` — a
  hardware-in-loop smoke test, not a host test.
