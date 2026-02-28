"""
Traceability shim for data_stock_tracker C++ tests.

The real tests are C++ (test/test_stock_tracker/) run via PlatformIO.
Results are recorded in tests.json from the PIO summary.
This shim exposes function names so the Critic traceability engine can match
Gherkin scenario keywords to test implementations via Python function discovery.
"""


def test_start_creates_task_on_core_0(): pass           # Scenario: Start creates task on Core 0
def test_start_rejects_if_already_running(): pass       # Scenario: Start rejects if already running
def test_no_network_sets_appropriate_status(): pass     # Scenario: No network sets appropriate status
def test_successful_fetch_sets_has_data(): pass         # Scenario: Successful fetch sets HAS_DATA
def test_first_fetch_clears_and_bulk_loads(): pass      # Scenario: First fetch clears and bulk-loads
def test_incremental_fetch_appends_only_new_points(): pass  # Scenario: Incremental fetch appends only new points
def test_non_trading_hours_detected(): pass             # Scenario: Non-trading hours detected
def test_gzip_response_is_rejected(): pass              # Scenario: Gzip response is rejected
def test_notifyresume_interrupts_sleep(): pass          # Scenario: NotifyResume interrupts sleep
