# Feature: HAL Network

> Label: "HAL: Network"
> Category: "HAL"
> Prerequisite: features/arch_hal_contract.md
> Prerequisite: features/arch_concurrency.md

[TODO]

## 1. Overview

The Network HAL provides a hardware-agnostic C interface for WiFi connectivity and HTTP communication on LPad devices. It manages the WiFi connection lifecycle with automatic reconnection, handles HTTP GET requests with chunked transfer decoding, and includes defensive workarounds for known ESP32 WiFi race conditions. The implementation runs on Core 0 alongside other background tasks.

---

## 2. Requirements

### 2.1 WiFi Lifecycle

- `hal_network_init(ssid, password)` MUST store credentials for later reconnection and begin asynchronous WiFi connection. Returns `false` if both parameters are null.
- `hal_network_get_status()` MUST drive the connection state machine and return the current status. It is side-effectful (advances timeouts, triggers retries).
- `hal_network_disconnect()` MUST fully shut down the WiFi radio via `esp_wifi_stop()` equivalent and reset the internal `wifi_started` flag.
- `hal_network_reconnect()` MUST use stored credentials and reset the retry counter. It MUST transition through `WIFI_OFF` state to avoid the `WiFi.disconnect(true)` race condition.
- `hal_network_get_ssid()` MUST return the connected SSID or `"N/A"` if not connected.

### 2.2 Auto-Reconnect State Machine

- Connection timeout: 10 seconds. After timeout, status transitions to ERROR.
- Auto-retry: up to 3 attempts at 5-second intervals after ERROR.
- After retries exhausted: periodic 60-second retry resets the counter.
- Successful connection resets the retry counter.
- External WiFi disconnect (radio drop) transitions to DISCONNECTED.

### 2.3 HTTP GET

- `hal_network_http_get(url, buffer, size)` MUST return `false` if WiFi is not connected, URL is null, buffer is null, or buffer size is 0.
- MUST reject responses where `Content-Length >= buffer_size` before reading.
- MUST use stream reads (not `getString()`) to avoid cross-core preemption byte loss.
- MUST send `Accept-Encoding: identity` header to prevent gzip (ESP32 HTTPClient cannot decompress).
- MUST handle chunked transfer encoding: if `Content-Length == -1`, the response body is decoded in-place via `decodeChunkedInPlace()`.
- Connection timeout: 10 seconds. Read stall timeout: 10 seconds.

### 2.4 WiFi Race Condition Handling

- Intentional disconnect paths MUST use `disconnect(true)` for full radio shutdown.
- Init and reconnect paths MUST use `disconnect(false)` to keep the radio alive, avoiding the `WIFI_EVENT_STA_START` race with `WiFi.begin()`.
- Reconnect MUST do `WiFi.mode(WIFI_OFF)` + 100ms delay + reset `wifi_started` flag before reinitializing.

### 2.5 Ping

- `hal_network_ping(host)` MUST return `true` for any positive HTTP response code (including 3xx/4xx), confirming network connectivity.
- If the host appears to be an IP address, it MUST use `http://google.com` instead (raw IP pinging is not meaningful for connectivity verification).

### 2.6 Stub Requirements

- The stub MUST compile on host-native without embedded SDK dependencies.
- `hal_network_init()` MUST immediately set status to CONNECTED and return `true`.
- `hal_network_http_get()` MUST set `buffer[0]='\0'` and return `false`.
- Test helpers: `hal_network_stub_set_status()` and `hal_network_stub_set_ping_result()`.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Init with null credentials fails

    Given no WiFi credentials are provided
    When hal_network_init(null, null) is called
    Then it returns false
    And hal_network_get_status returns HAL_NETWORK_STATUS_ERROR

#### Scenario: Init stores credentials for reconnect

    Given valid WiFi credentials are provided
    When hal_network_init is called
    Then it returns true
    And credentials are stored for subsequent reconnect calls

#### Scenario: Status polling drives connection state machine

    Given hal_network_init has been called with valid credentials
    When hal_network_get_status is polled repeatedly
    Then the status progresses from CONNECTING to CONNECTED when WiFi associates
    Or the status progresses from CONNECTING to ERROR after 10s timeout

#### Scenario: Auto-retry after connection failure

    Given a connection attempt has timed out (status ERROR)
    When hal_network_get_status is polled after 5 seconds
    Then a retry attempt is initiated
    And up to 3 retries are attempted before the 60s periodic retry cycle begins

#### Scenario: HTTP GET rejects when not connected

    Given hal_network_get_status returns DISCONNECTED
    When hal_network_http_get is called
    Then it returns false

#### Scenario: HTTP GET rejects null buffer

    Given WiFi is connected
    When hal_network_http_get is called with a null buffer
    Then it returns false

#### Scenario: HTTP GET rejects oversized response

    Given WiFi is connected
    And the server responds with Content-Length exceeding the buffer size
    When hal_network_http_get is called
    Then it returns false without reading the response body

#### Scenario: Chunked transfer encoding is decoded in-place

    Given WiFi is connected
    And the server responds with Transfer-Encoding: chunked
    When hal_network_http_get is called
    Then the chunked encoding is decoded in-place in the response buffer
    And the buffer contains the assembled response body

#### Scenario: Get SSID returns N/A when disconnected

    Given WiFi is not connected
    When hal_network_get_ssid is called
    Then it returns "N/A"

#### Scenario: Stub init succeeds immediately

    Given the native_test stub is active
    When hal_network_init is called with any credentials
    Then it returns true
    And status is immediately CONNECTED

#### Scenario: Stub HTTP GET returns empty

    Given the native_test stub is active
    When hal_network_http_get is called
    Then it returns false
    And the buffer is empty

### Manual Scenarios (Human Verification Required)

#### Scenario: WiFi connects on device boot

    Given the device has valid WiFi credentials in config.json
    When the device is powered on
    Then it connects to the configured WiFi network within 10 seconds

#### Scenario: WiFi auto-reconnects after signal loss

    Given the device is connected to WiFi
    When the WiFi access point is temporarily disabled for 15 seconds
    And the access point is re-enabled
    Then the device reconnects automatically within 60 seconds
