#ifndef RUNTIME_CONFIG_H
#define RUNTIME_CONFIG_H

#ifndef GH_WIFI_SSID
#define GH_WIFI_SSID ""
#endif

#ifndef GH_WIFI_PASSWORD
#define GH_WIFI_PASSWORD ""
#endif

#ifndef GH_MQTT_BROKER_URI
#define GH_MQTT_BROKER_URI ""
#endif

#ifndef GH_HEARTBEAT_INTERVAL_MS
#define GH_HEARTBEAT_INTERVAL_MS 30000
#endif

#ifndef GH_WIFI_RETRY_BASE_MS
#define GH_WIFI_RETRY_BASE_MS 1000
#endif

#ifndef GH_WIFI_RETRY_MAX_MS
#define GH_WIFI_RETRY_MAX_MS 30000
#endif

#ifndef GH_MQTT_RETRY_BASE_MS
#define GH_MQTT_RETRY_BASE_MS 1000
#endif

#ifndef GH_MQTT_RETRY_MAX_MS
#define GH_MQTT_RETRY_MAX_MS 30000
#endif

#endif
