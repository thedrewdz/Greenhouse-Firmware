import json
import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
EDGE_ROOT = REPO_ROOT / "greenhouse-edge"
SRC_ROOT = EDGE_ROOT / "src"
LOCAL_DEVICE_ID = "1ADD5912AF61"


def read_source(relative_path: str) -> str:
    return (REPO_ROOT / relative_path).read_text(encoding="utf-8")


def runtime_define(name: str) -> int:
    source = read_source("greenhouse-edge/src/runtime_config.h")
    match = re.search(rf"#define\s+{re.escape(name)}\s+(\d+)", source)
    if match is None:
        raise AssertionError(f"{name} not found in runtime_config.h")
    return int(match.group(1))


def backoff_without_jitter(base_ms: int, max_ms: int, completed_failures: int) -> int:
    backoff = base_ms
    for _ in range(max(completed_failures - 1, 0)):
        backoff *= 2
        if backoff > max_ms:
            backoff = max_ms
    return backoff


def jitter_bounds(backoff_ms: int) -> tuple[int, int]:
    jitter_window = backoff_ms // 5
    return backoff_ms - jitter_window, backoff_ms + jitter_window


def parse_provisioning_payload(payload: str, local_device_id: str = LOCAL_DEVICE_ID) -> tuple[bool, int, dict]:
    try:
        root = json.loads(payload)
    except json.JSONDecodeError:
        return False, 2001, {}

    if not isinstance(root.get("schema_version"), int) or root["schema_version"] != 1:
        return False, 2001, {}

    if root.get("device_id") != local_device_id:
        return False, 2002, {}

    wifi_ssid = root.get("wifi_ssid")
    if not isinstance(wifi_ssid, str) or wifi_ssid == "":
        return False, 2003, {}

    wifi_password = root.get("wifi_password")
    if not isinstance(wifi_password, str):
        return False, 2099, {}

    mqtt_broker_uri = root.get("mqtt_broker_uri")
    if not isinstance(mqtt_broker_uri, str):
        return False, 2004, {}
    if not (
        mqtt_broker_uri.startswith("mqtt://")
        and len(mqtt_broker_uri) > len("mqtt://")
        and mqtt_broker_uri[len("mqtt://")] != "/"
    ) and not (
        mqtt_broker_uri.startswith("mqtts://")
        and len(mqtt_broker_uri) > len("mqtts://")
        and mqtt_broker_uri[len("mqtts://")] != "/"
    ):
        return False, 2004, {}

    heartbeat_interval_ms = root.get("heartbeat_interval_ms", runtime_define("GH_HEARTBEAT_INTERVAL_MS"))
    if not isinstance(heartbeat_interval_ms, int) or heartbeat_interval_ms < 0:
        return False, 2099, {}

    return True, 0, {
        "wifi_ssid": wifi_ssid,
        "wifi_password": wifi_password,
        "mqtt_broker_uri": mqtt_broker_uri,
        "heartbeat_interval_ms": heartbeat_interval_ms,
    }


class OnboardingRetryPolicyTests(unittest.TestCase):
    def test_bootstrap_retry_policy_matches_canonical_schedule(self) -> None:
        self.assertEqual(runtime_define("GH_BOOTSTRAP_RETRY_BUDGET"), 5)

        wifi_base = runtime_define("GH_WIFI_RETRY_BASE_MS")
        wifi_max = runtime_define("GH_WIFI_RETRY_MAX_MS")
        mqtt_base = runtime_define("GH_MQTT_RETRY_BASE_MS")
        mqtt_max = runtime_define("GH_MQTT_RETRY_MAX_MS")

        self.assertEqual(
            [backoff_without_jitter(wifi_base, wifi_max, failures) for failures in range(1, 5)],
            [1000, 2000, 4000, 8000],
        )
        self.assertEqual(
            [backoff_without_jitter(mqtt_base, mqtt_max, failures) for failures in range(1, 5)],
            [1000, 2000, 4000, 8000],
        )

    def test_bootstrap_retry_jitter_bounds_are_twenty_percent(self) -> None:
        for expected_delay in (1000, 2000, 4000, 8000):
            self.assertEqual(jitter_bounds(expected_delay), (expected_delay * 4 // 5, expected_delay * 6 // 5))

    def test_firmware_schedules_backoff_from_completed_failures(self) -> None:
        network_source = read_source("greenhouse-edge/src/services_network.c")
        mqtt_source = read_source("greenhouse-edge/src/services_mqtt.c")

        self.assertIn("compute_backoff_ms((s_retry_count > 0U) ? (s_retry_count - 1U) : 0U)", network_source)
        self.assertIn("compute_backoff_ms((s_retry_count > 0U) ? (s_retry_count - 1U) : 0U)", mqtt_source)

    def test_provisioning_reentry_resets_wifi_and_mqtt_before_ble_advertising(self) -> None:
        app_source = read_source("greenhouse-edge/src/app.c")
        entrypoint = app_source.index("static void enter_provisioning_mode")
        mqtt_reset = app_source.index("gh_mqtt_stop_reset();", entrypoint)
        network_reset = app_source.index("gh_network_stop_reset();", entrypoint)
        ble_start = app_source.index("gh_ble_onboarding_start", entrypoint)

        self.assertLess(mqtt_reset, ble_start)
        self.assertLess(network_reset, ble_start)

    def test_mqtt_init_recreates_client_for_new_bootstrap_endpoint(self) -> None:
        mqtt_source = read_source("greenhouse-edge/src/services_mqtt.c")
        init_start = mqtt_source.index("esp_err_t gh_mqtt_init")
        reset_call = mqtt_source.index("gh_mqtt_stop_reset();", init_start)
        client_init = mqtt_source.index("esp_mqtt_client_init", init_start)

        self.assertLess(reset_call, client_init)

    def test_success_status_is_not_stopped_before_ble_notification(self) -> None:
        app_source = read_source("greenhouse-edge/src/app.c")
        callback_start = app_source.index("static void on_provisioning_payload")
        callback_end = app_source.index("void gh_app_init", callback_start)
        callback_body = app_source[callback_start:callback_end]
        ble_source = read_source("greenhouse-edge/src/services_ble_onboarding.c")
        access_start = ble_source.index("static int on_gatt_access")
        handled_log = ble_source.index("BLE provisioning payload handled", access_start)
        notify_call = ble_source.index("notify_status();", handled_log)
        stop_call = ble_source.index("gh_ble_onboarding_stop();", notify_call)

        self.assertNotIn("gh_ble_onboarding_stop();", callback_body)
        self.assertLess(notify_call, stop_call)

    def test_provisioning_persistence_uses_shadow_slot_promotion(self) -> None:
        provisioning_source = read_source("greenhouse-edge/src/services_provisioning_config.c")
        save_start = provisioning_source.index("esp_err_t gh_provisioning_config_save")
        save_body = provisioning_source[save_start:]
        candidate_write = provisioning_source.index("save_to_namespace(slot_namespace(candidate_slot), config)", save_start)
        promotion = provisioning_source.index("promote_active_slot(candidate_slot)", save_start)

        self.assertIn("KEY_ACTIVE_SLOT", provisioning_source)
        self.assertIn("NVS_SLOT_A_NAMESPACE", provisioning_source)
        self.assertIn("NVS_SLOT_B_NAMESPACE", provisioning_source)
        self.assertLess(candidate_write, promotion)
        self.assertNotIn("nvs_set_str(handle, KEY_WIFI_SSID", save_body)


class ProvisioningPayloadContractTests(unittest.TestCase):
    def payload(self, **overrides: object) -> str:
        data: dict[str, object] = {
            "schema_version": 1,
            "device_id": LOCAL_DEVICE_ID,
            "wifi_ssid": "ExampleWiFi",
            "wifi_password": "ExamplePassword",
            "mqtt_broker_uri": "mqtt://192.168.1.50",
        }
        data.update(overrides)
        return json.dumps(data)

    def test_valid_payload_uses_default_heartbeat_interval(self) -> None:
        ok, error_code, config = parse_provisioning_payload(self.payload())

        self.assertTrue(ok)
        self.assertEqual(error_code, 0)
        self.assertEqual(config["heartbeat_interval_ms"], runtime_define("GH_HEARTBEAT_INTERVAL_MS"))

    def test_valid_payload_accepts_empty_password_and_explicit_heartbeat(self) -> None:
        ok, error_code, config = parse_provisioning_payload(
            self.payload(wifi_password="", heartbeat_interval_ms=45000)
        )

        self.assertTrue(ok)
        self.assertEqual(error_code, 0)
        self.assertEqual(config["wifi_password"], "")
        self.assertEqual(config["heartbeat_interval_ms"], 45000)

    def test_invalid_payloads_map_to_single_canonical_error_code(self) -> None:
        cases = [
            ("unsupported schema", self.payload(schema_version=2), 2001),
            ("device mismatch", self.payload(device_id="FFFFFFFFFFFF"), 2002),
            ("empty ssid", self.payload(wifi_ssid=""), 2003),
            ("missing mqtt uri", self.payload(mqtt_broker_uri=None), 2004),
            ("malformed mqtt uri", self.payload(mqtt_broker_uri="http://192.168.1.50"), 2004),
        ]

        for name, payload, expected_error_code in cases:
            with self.subTest(name=name):
                ok, error_code, config = parse_provisioning_payload(payload)
                self.assertFalse(ok)
                self.assertEqual(error_code, expected_error_code)
                self.assertEqual(config, {})


if __name__ == "__main__":
    unittest.main()
