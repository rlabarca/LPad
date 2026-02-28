"""
Traceability shim for hal_network C++ tests.

The real tests are C++ (test/test_network_hal/) run via PlatformIO.
Results are recorded in tests.json from the PIO summary.
"""


def test_init_with_null_credentials_fails(): pass
def test_init_stores_credentials_for_reconnect(): pass
def test_status_polling_drives_connection_state_machine(): pass
def test_auto_retry_after_connection_failure(): pass
def test_http_get_rejects_when_not_connected(): pass
def test_http_get_rejects_null_buffer(): pass
def test_http_get_rejects_oversized_response(): pass
def test_chunked_transfer_encoding_is_decoded_in_place(): pass
def test_get_ssid_returns_na_when_disconnected(): pass
def test_stub_init_succeeds_immediately(): pass
def test_stub_http_get_returns_empty(): pass
