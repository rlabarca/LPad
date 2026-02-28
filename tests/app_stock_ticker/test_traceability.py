# Traceability manifest for app_stock_ticker.
# Links Gherkin automated scenarios to C++ Unity tests in:
#   test/test_stock_ticker_app/test_stock_ticker_app.cpp
#
# Function names mirror the C++ test names; this file has no runtime
# behavior — it exists solely for the Critic's keyword-matching engine.

def test_begin_creates_tracker_and_graph(): pass
def test_status_screen_shown_when_waiting_with_no_data(): pass
def test_non_trading_hours_always_shown(): pass
def test_graph_renders_when_data_available(): pass
def test_pause_stops_tracker_before_wifi_teardown(): pass
def test_unpause_forces_full_redraw(): pass
