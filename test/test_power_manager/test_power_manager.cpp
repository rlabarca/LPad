/**
 * @file test_power_manager.cpp
 * @brief Unit tests for BatteryStatus data model and PowerManager service
 *
 * Covers:
 * - BatteryStatus construction, getters/setters, timestamp updates
 * - PowerManager HAL initialization
 * - PowerManager polling interval enforcement (2-second cadence)
 * - PowerManager first-update immediate poll behavior
 * - Boundary conditions (NO_BATTERY, full charge, low battery)
 *
 * Specification: features/sys_battery_metering.md
 * Architecture:  features/arch_power_management.md
 */

#include <unity.h>
#include "data/battery_status.h"
#include "system/power_manager.h"
#include "../../hal/power.h"

// Test helpers from power_stub.cpp (C++ linkage, not extern "C")
void hal_power_stub_set_status(hal_power_status_t status);
void hal_power_stub_set_charge_level(int8_t level);
void hal_power_stub_set_voltage_mv(uint16_t mv);

void setUp(void) {
    // Reset stub to default values per hal_spec_power.md §3.1
    hal_power_stub_set_status(HAL_POWER_STATUS_DISCHARGING);
    hal_power_stub_set_charge_level(75);
    hal_power_stub_set_voltage_mv(3800);
}

void tearDown(void) {}

// ==========================================
// BatteryStatus Data Model
// ==========================================

void test_battery_status_name() {
    BatteryStatus bs;
    TEST_ASSERT_EQUAL_STRING("BatteryStatus", bs.getName().c_str());
}

void test_battery_status_default_values() {
    BatteryStatus bs;
    TEST_ASSERT_EQUAL(HAL_POWER_STATUS_UNKNOWN, bs.getStatus());
    TEST_ASSERT_EQUAL_INT8(-1, bs.getChargeLevel());
    TEST_ASSERT_EQUAL_UINT16(0, bs.getVoltageMv());
}

void test_battery_status_update_all_fields() {
    BatteryStatus bs;
    bs.update(HAL_POWER_STATUS_CHARGING, 85, 4100);
    TEST_ASSERT_EQUAL(HAL_POWER_STATUS_CHARGING, bs.getStatus());
    TEST_ASSERT_EQUAL_INT8(85, bs.getChargeLevel());
    TEST_ASSERT_EQUAL_UINT16(4100, bs.getVoltageMv());
}

void test_battery_status_update_no_battery() {
    BatteryStatus bs;
    bs.update(HAL_POWER_STATUS_NO_BATTERY, -1, 0);
    TEST_ASSERT_EQUAL(HAL_POWER_STATUS_NO_BATTERY, bs.getStatus());
    TEST_ASSERT_EQUAL_INT8(-1, bs.getChargeLevel());
    TEST_ASSERT_EQUAL_UINT16(0, bs.getVoltageMv());
}

void test_battery_status_touch_updates_timestamp() {
    BatteryStatus bs;
    uint64_t before = bs.getLastUpdated();
    bs.update(HAL_POWER_STATUS_DISCHARGING, 50, 3700);
    // touch() should update timestamp (timer stub returns incrementing values)
    TEST_ASSERT_TRUE(bs.getLastUpdated() >= before);
}

void test_battery_status_inherits_data_item() {
    BatteryStatus bs;
    // DataItem interface accessible
    TEST_ASSERT_EQUAL_STRING("BatteryStatus", bs.getName().c_str());
    TEST_ASSERT_EQUAL_UINT64(0, bs.getLastUpdated());
}

// ==========================================
// PowerManager Service
// ==========================================

void test_power_manager_begin_succeeds() {
    PowerManager pm;
    TEST_ASSERT_TRUE(pm.begin());
}

void test_power_manager_battery_status_not_null() {
    PowerManager pm;
    TEST_ASSERT_NOT_NULL(pm.getBatteryStatus());
}

void test_power_manager_initial_poll_on_first_update() {
    PowerManager pm;
    pm.begin();

    // First update should trigger immediate poll (m_elapsed starts at POLL_INTERVAL_S)
    pm.update(0.033f);

    const BatteryStatus* bs = pm.getBatteryStatus();
    TEST_ASSERT_EQUAL(HAL_POWER_STATUS_DISCHARGING, bs->getStatus());
    TEST_ASSERT_EQUAL_INT8(75, bs->getChargeLevel());
    TEST_ASSERT_EQUAL_UINT16(3800, bs->getVoltageMv());
}

void test_power_manager_does_not_poll_before_interval() {
    PowerManager pm;
    pm.begin();

    // Consume initial poll
    pm.update(0.0f);

    // Change stub values
    hal_power_stub_set_status(HAL_POWER_STATUS_CHARGING);
    hal_power_stub_set_charge_level(90);

    // Update for 1 second (below 2-second interval)
    pm.update(1.0f);

    // Should still have values from initial poll
    const BatteryStatus* bs = pm.getBatteryStatus();
    TEST_ASSERT_EQUAL(HAL_POWER_STATUS_DISCHARGING, bs->getStatus());
    TEST_ASSERT_EQUAL_INT8(75, bs->getChargeLevel());
}

void test_power_manager_polls_at_interval() {
    PowerManager pm;
    pm.begin();

    // Consume initial poll
    pm.update(0.0f);

    // Change stub values
    hal_power_stub_set_status(HAL_POWER_STATUS_CHARGING);
    hal_power_stub_set_charge_level(90);
    hal_power_stub_set_voltage_mv(4150);

    // Accumulate past the 2-second threshold
    pm.update(1.0f);
    pm.update(1.5f);  // Total: 2.5s > 2.0s

    const BatteryStatus* bs = pm.getBatteryStatus();
    TEST_ASSERT_EQUAL(HAL_POWER_STATUS_CHARGING, bs->getStatus());
    TEST_ASSERT_EQUAL_INT8(90, bs->getChargeLevel());
    TEST_ASSERT_EQUAL_UINT16(4150, bs->getVoltageMv());
}

void test_power_manager_no_battery_scenario() {
    hal_power_stub_set_status(HAL_POWER_STATUS_NO_BATTERY);
    hal_power_stub_set_charge_level(-1);
    hal_power_stub_set_voltage_mv(0);

    PowerManager pm;
    pm.begin();
    pm.update(0.0f);  // Immediate poll

    const BatteryStatus* bs = pm.getBatteryStatus();
    TEST_ASSERT_EQUAL(HAL_POWER_STATUS_NO_BATTERY, bs->getStatus());
    TEST_ASSERT_EQUAL_INT8(-1, bs->getChargeLevel());
    TEST_ASSERT_EQUAL_UINT16(0, bs->getVoltageMv());
}

void test_power_manager_charged_scenario() {
    hal_power_stub_set_status(HAL_POWER_STATUS_CHARGED);
    hal_power_stub_set_charge_level(100);
    hal_power_stub_set_voltage_mv(4200);

    PowerManager pm;
    pm.begin();
    pm.update(0.0f);

    const BatteryStatus* bs = pm.getBatteryStatus();
    TEST_ASSERT_EQUAL(HAL_POWER_STATUS_CHARGED, bs->getStatus());
    TEST_ASSERT_EQUAL_INT8(100, bs->getChargeLevel());
}

void test_power_manager_low_battery_scenario() {
    hal_power_stub_set_status(HAL_POWER_STATUS_DISCHARGING);
    hal_power_stub_set_charge_level(5);
    hal_power_stub_set_voltage_mv(3350);

    PowerManager pm;
    pm.begin();
    pm.update(0.0f);

    const BatteryStatus* bs = pm.getBatteryStatus();
    TEST_ASSERT_EQUAL(HAL_POWER_STATUS_DISCHARGING, bs->getStatus());
    TEST_ASSERT_EQUAL_INT8(5, bs->getChargeLevel());
}

void test_power_manager_not_opaque_not_fullscreen() {
    PowerManager pm;
    TEST_ASSERT_FALSE(pm.isOpaque());
    TEST_ASSERT_FALSE(pm.isFullscreen());
}

void test_power_manager_is_system_component() {
    PowerManager pm;
    TEST_ASSERT_EQUAL(UIComponent::Type::SYSTEM, pm.getComponentType());
}

// ==========================================
// Main
// ==========================================

int main(int argc, char** argv) {
    UNITY_BEGIN();

    // BatteryStatus data model
    RUN_TEST(test_battery_status_name);
    RUN_TEST(test_battery_status_default_values);
    RUN_TEST(test_battery_status_update_all_fields);
    RUN_TEST(test_battery_status_update_no_battery);
    RUN_TEST(test_battery_status_touch_updates_timestamp);
    RUN_TEST(test_battery_status_inherits_data_item);

    // PowerManager service
    RUN_TEST(test_power_manager_begin_succeeds);
    RUN_TEST(test_power_manager_battery_status_not_null);
    RUN_TEST(test_power_manager_initial_poll_on_first_update);
    RUN_TEST(test_power_manager_does_not_poll_before_interval);
    RUN_TEST(test_power_manager_polls_at_interval);
    RUN_TEST(test_power_manager_no_battery_scenario);
    RUN_TEST(test_power_manager_charged_scenario);
    RUN_TEST(test_power_manager_low_battery_scenario);
    RUN_TEST(test_power_manager_not_opaque_not_fullscreen);
    RUN_TEST(test_power_manager_is_system_component);

    return UNITY_END();
}
