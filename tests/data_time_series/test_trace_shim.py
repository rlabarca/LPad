"""
Traceability shim for data_time_series C++ tests.

The real tests are C++ (test/test_data_series/) run via PlatformIO.
Results are recorded in tests.json from the PIO summary.
This shim exposes function names so the Critic traceability engine can match
Gherkin scenario keywords to test implementations via Python function discovery.
"""


def test_add_points_up_to_capacity(): pass          # Scenario: Add points up to capacity
def test_fifo_eviction_when_full(): pass             # Scenario: FIFO eviction when full
def test_min_max_tracks_current_values(): pass       # Scenario: Min/max tracks current values
def test_min_max_recalculates_on_eviction_of_extreme(): pass  # Scenario: Min/max recalculates on eviction of extreme
def test_graph_data_snapshot_is_chronological(): pass         # Scenario: Graph data snapshot is chronological
def test_clear_resets_all_state(): pass              # Scenario: Clear resets all state
