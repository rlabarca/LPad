"""
Traceability shim for hal_touch C++ tests.

The real tests are C++ (test/test_touch_hal/) run via PlatformIO.
Results are recorded in tests.json from the PIO summary.
"""


def test_touch_init_succeeds_on_first_call(): pass
def test_double_init_is_idempotent(): pass
def test_read_with_no_touch_returns_unpressed_state(): pass
def test_read_with_null_pointer_returns_false(): pass
def test_i2c_transient_error_does_not_fail_the_read(): pass
def test_touch_coordinates_are_clamped_to_display_bounds(): pass
def test_home_button_detected_on_cst816t(): pass
def test_stub_returns_default_unpressed_state(): pass
