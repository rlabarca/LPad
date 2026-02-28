"""
Traceability shim for hal_power C++ tests.

The real tests are C++ (test/test_hal_power/) run via PlatformIO.
Results are recorded in tests.json from the PIO summary.
"""


def test_power_init_is_non_fatal_without_pmu(): pass
def test_charge_level_maps_voltage_to_percentage(): pass
def test_below_minimum_voltage_reports_no_battery(): pass
def test_charge_level_rate_limited_while_charging(): pass
def test_charge_level_frozen_while_discharging(): pass
def test_button_event_is_consume_on_read(): pass
def test_stub_default_state(): pass
def test_stub_test_helper_overrides_charge_level(): pass
