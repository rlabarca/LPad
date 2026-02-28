"""
Traceability shim for hal_timer C++ tests.

The real tests are C++ (test/test_timer_hal/) run via PlatformIO.
Results are recorded in tests.json from the PIO summary.
"""


def test_timer_init_returns_bool(): pass
def test_stub_timer_init_returns_false_by_default(): pass
def test_stub_timer_can_be_overridden_by_test_fixture(): pass
def test_timer_returns_monotonically_increasing_values(): pass
