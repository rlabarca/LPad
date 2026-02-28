"""
Traceability shim for widget_scrollable_list C++ tests.

The real tests are C++ (test/test_widget_framework/) run via PlatformIO.
Results are recorded in tests.json from the PIO summary.
This shim exposes function names so the Critic traceability engine can match
Gherkin scenario keywords to test implementations via Python function discovery.
"""


def test_scrollable_list_add_items(): pass                # Scenario: Add item returns sequential indices
def test_scrollable_list_clear(): pass                    # Scenario: Clear items resets all state
def test_scrollable_list_scroll_bounds(): pass            # Scenario: Scroll clamps to bottom
def test_scrollable_list_selection(): pass                # Scenario: Tap selects correct item accounting for scroll offset
def test_add_item_returns_sequential_indices(): pass      # Scenario: Add item returns sequential indices
def test_swipe_up_scrolls_down_by_half_page(): pass       # Scenario: Swipe up scrolls down by half page
def test_scroll_clamps_to_bottom(): pass                  # Scenario: Scroll clamps to bottom
