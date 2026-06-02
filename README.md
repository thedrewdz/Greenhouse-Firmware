# Greenhouse Firmware

Firmware and supporting documentation for Greenhouse Edge Units and related platform integration.

## MQTT Broker Reference

The Main Unit currently uses Mosquitto as the local MQTT broker on the Raspberry Pi.

Typical service commands on the Pi:

```sh
sudo systemctl status mosquitto
sudo systemctl start mosquitto
sudo systemctl enable mosquitto
sudo journalctl -u mosquitto -n 100 --no-pager
```

Notes:

- `systemctl status` checks whether the broker is installed and running.
- `systemctl start` starts the broker immediately.
- `systemctl enable` configures the broker to start on boot.
- `journalctl` shows broker service logs for startup, bind, auth, or config issues.

## Viewing MQTT Traffic

`mosquitto_sub` does not start the broker. It subscribes to live MQTT traffic.

Useful commands:

```sh
mosquitto_sub -h localhost -t "gh/heartbeat" -v
mosquitto_sub -h localhost -t "gh/#" -v
```

Notes:

- `gh/heartbeat` shows the current Phase 1 Edge Unit heartbeat topic.
- `gh/#` shows all current topics under the `gh` namespace.
- Use these commands to confirm that the Edge Unit is reaching the broker and publishing payloads.

See also:

- `docs/mqtt-topics.md`
- `docs/spec-heartbeat-phase1-skeleton.md`
