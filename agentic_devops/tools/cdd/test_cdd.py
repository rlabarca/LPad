#!/usr/bin/env python3
"""Local test suite for the CDD Status Monitor.

Tests the core logic in serve.py:
  1. DevOps aggregated test status (scan tools/*/test_status.json)
  2. PlatformIO test status reader
  3. Domain dispatch (get_domain_test_status)
  4. DONE capping logic

Generates test_status.json per the Builder Instructions reporting protocol.
"""

import json
import os
import sys
import tempfile
import shutil
from datetime import datetime

# Add this tool's directory so we can import serve
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from serve import (
    get_devops_aggregated_test_status,
    get_pio_test_status,
    get_domain_test_status,
    DONE_CAP,
)

PASS_COUNT = 0
FAIL_COUNT = 0
FAILURES = []


def assert_eq(test_name, actual, expected):
    global PASS_COUNT, FAIL_COUNT
    if actual == expected:
        PASS_COUNT += 1
        print(f"  PASS: {test_name}")
    else:
        FAIL_COUNT += 1
        FAILURES.append(test_name)
        print(f"  FAIL: {test_name}")
        print(f"    expected: {expected}")
        print(f"    actual:   {actual}")


# --- Test: DevOps Aggregated Test Status ---

def test_aggregate_all_pass():
    """All tools report PASS -> overall PASS."""
    with tempfile.TemporaryDirectory() as tmpdir:
        for name in ["tool_a", "tool_b"]:
            d = os.path.join(tmpdir, name)
            os.makedirs(d)
            with open(os.path.join(d, "test_status.json"), "w") as f:
                json.dump({"status": "PASS", "timestamp": "2026-01-01", "message": "ok"}, f)

        status, msg = get_devops_aggregated_test_status(tmpdir)
        assert_eq("aggregate_all_pass.status", status, "PASS")
        assert_eq("aggregate_all_pass.msg", msg, "All tools nominal")


def test_aggregate_one_fail():
    """One FAIL among passes -> overall FAIL, naming the failing tool."""
    with tempfile.TemporaryDirectory() as tmpdir:
        for name, st in [("tool_a", "PASS"), ("tool_b", "FAIL"), ("tool_c", "PASS")]:
            d = os.path.join(tmpdir, name)
            os.makedirs(d)
            with open(os.path.join(d, "test_status.json"), "w") as f:
                json.dump({"status": st}, f)

        status, msg = get_devops_aggregated_test_status(tmpdir)
        assert_eq("aggregate_one_fail.status", status, "FAIL")
        assert_eq("aggregate_one_fail.msg_contains_tool_b", "tool_b" in msg, True)


def test_aggregate_no_status_files():
    """No test_status.json files found -> UNKNOWN."""
    with tempfile.TemporaryDirectory() as tmpdir:
        os.makedirs(os.path.join(tmpdir, "tool_a"))  # dir exists but no json

        status, msg = get_devops_aggregated_test_status(tmpdir)
        assert_eq("aggregate_no_files.status", status, "UNKNOWN")


def test_aggregate_missing_dir():
    """Non-existent tools directory -> UNKNOWN."""
    status, msg = get_devops_aggregated_test_status("/nonexistent/path")
    assert_eq("aggregate_missing_dir.status", status, "UNKNOWN")


def test_aggregate_malformed_json():
    """Malformed JSON in test_status.json -> treated as FAIL for that tool."""
    with tempfile.TemporaryDirectory() as tmpdir:
        d = os.path.join(tmpdir, "bad_tool")
        os.makedirs(d)
        with open(os.path.join(d, "test_status.json"), "w") as f:
            f.write("{not valid json")

        status, msg = get_devops_aggregated_test_status(tmpdir)
        assert_eq("aggregate_malformed.status", status, "FAIL")
        assert_eq("aggregate_malformed.msg_contains_bad_tool", "bad_tool" in msg, True)


def test_aggregate_ignores_dirs_without_status():
    """Dirs without test_status.json are silently ignored."""
    with tempfile.TemporaryDirectory() as tmpdir:
        # One tool with status, one without
        d_with = os.path.join(tmpdir, "tool_a")
        os.makedirs(d_with)
        with open(os.path.join(d_with, "test_status.json"), "w") as f:
            json.dump({"status": "PASS"}, f)

        d_without = os.path.join(tmpdir, "tool_b")
        os.makedirs(d_without)  # no test_status.json

        status, msg = get_devops_aggregated_test_status(tmpdir)
        assert_eq("aggregate_ignores_missing.status", status, "PASS")


# --- Test: PlatformIO Test Status ---

def test_pio_pass():
    """PIO summary with zero errors and failures -> PASS."""
    with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f:
        f.write('{"error_nums": 0, "failure_nums": 0}')
        f.flush()
        status, msg = get_pio_test_status(f.name)
    os.unlink(f.name)
    assert_eq("pio_pass.status", status, "PASS")


def test_pio_fail():
    """PIO summary with failures -> FAIL."""
    with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f:
        f.write('{"error_nums": 0, "failure_nums": 2}')
        f.flush()
        status, msg = get_pio_test_status(f.name)
    os.unlink(f.name)
    assert_eq("pio_fail.status", status, "FAIL")


def test_pio_missing():
    """Missing PIO summary file -> UNKNOWN."""
    status, msg = get_pio_test_status("/nonexistent/summary.json")
    assert_eq("pio_missing.status", status, "UNKNOWN")


# --- Test: Domain Dispatch ---

def test_dispatch_devops():
    """DevOps domain config dispatches to aggregation."""
    with tempfile.TemporaryDirectory() as tmpdir:
        d = os.path.join(tmpdir, "tool_a")
        os.makedirs(d)
        with open(os.path.join(d, "test_status.json"), "w") as f:
            json.dump({"status": "PASS"}, f)

        domain = {"test_mode": "devops_aggregate", "tools_dir": tmpdir}
        status, msg = get_domain_test_status(domain)
        assert_eq("dispatch_devops.status", status, "PASS")


def test_dispatch_pio():
    """PIO domain config dispatches to PIO reader."""
    domain = {"test_mode": "pio_summary", "test_summary": "/nonexistent"}
    status, msg = get_domain_test_status(domain)
    assert_eq("dispatch_pio.status", status, "UNKNOWN")


# --- Test: DONE Capping Constant ---

def test_done_cap_value():
    """DONE_CAP must be 10 per the spec."""
    assert_eq("done_cap_value", DONE_CAP, 10)


def test_done_capping_logic():
    """Simulates the capping logic from _domain_column_html."""
    done_tuples = [(f"feature_{i}.md", 1000 + i) for i in range(15)]
    done_tuples.sort(key=lambda x: x[1], reverse=True)

    visible = [name for name, _ in done_tuples[:DONE_CAP]]
    overflow = len(done_tuples) - DONE_CAP

    assert_eq("capping.visible_count", len(visible), 10)
    assert_eq("capping.overflow", overflow, 5)
    assert_eq("capping.first_visible", visible[0], "feature_14.md")


# --- Run All ---

def main():
    print("=" * 50)
    print("CDD Monitor Test Suite")
    print("=" * 50)

    print("\n[Aggregation]")
    test_aggregate_all_pass()
    test_aggregate_one_fail()
    test_aggregate_no_status_files()
    test_aggregate_missing_dir()
    test_aggregate_malformed_json()
    test_aggregate_ignores_dirs_without_status()

    print("\n[PIO Test Status]")
    test_pio_pass()
    test_pio_fail()
    test_pio_missing()

    print("\n[Domain Dispatch]")
    test_dispatch_devops()
    test_dispatch_pio()

    print("\n[DONE Capping]")
    test_done_cap_value()
    test_done_capping_logic()

    print("\n" + "=" * 50)
    total = PASS_COUNT + FAIL_COUNT
    print(f"Results: {PASS_COUNT}/{total} passed, {FAIL_COUNT} failed")

    if FAILURES:
        print(f"Failures: {', '.join(FAILURES)}")

    # Generate test_status.json per reporting protocol
    script_dir = os.path.dirname(os.path.abspath(__file__))
    report = {
        "status": "PASS" if FAIL_COUNT == 0 else "FAIL",
        "timestamp": datetime.now().isoformat(),
        "message": f"{PASS_COUNT}/{total} passed" + (f", failures: {', '.join(FAILURES)}" if FAILURES else ""),
    }
    report_path = os.path.join(script_dir, "test_status.json")
    with open(report_path, "w") as f:
        json.dump(report, f, indent=2)
    print(f"\nReport written to {report_path}")

    return 0 if FAIL_COUNT == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
