# HAL Network -- Implementation Notes

### Source Mapping

| File | Role |
|---|---|
| `hal/network.h` | C interface contract |
| `hal/network_esp32.cpp` | ESP32 WiFi + HTTPClient implementation |
| `hal/network_stub.cpp` | Host-native test stub |
| `test/test_stock_tracker/` | Tests that exercise network stub |

### WiFi Disconnect Race Condition

`WiFi.disconnect(true)` calls `esp_wifi_stop()` asynchronously. A subsequent `WiFi.begin()` can race the `WIFI_EVENT_STA_START` event, causing "STA config failed". Solution: intentional disconnect uses `disconnect(true)`, but reconnect uses `disconnect(false)` + `WIFI_OFF` mode + 100ms delay.

### Chunked Transfer Decoder

`decodeChunkedInPlace()` works in-place with no extra allocation. First-byte hex-digit check (returns 0 if not chunked). Uses `memmove` to shift data left. Terminates on chunk size 0.

### Stream Reads

`getString()` was abandoned in favor of stream reads because cross-core preemption on the ESP32 could lose bytes during the string assembly.
