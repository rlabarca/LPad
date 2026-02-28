"""
Traceability shim for hal_display C++ tests.

The real tests are C++ (test/test_display_hal/, test/test_display_rotation/)
run via PlatformIO. Results are recorded in tests.json from the PIO summary.
This shim exposes function names so the Critic traceability engine can match
Gherkin scenario keywords to test implementations via Python function discovery.
"""


def test_hal_display_init_returns_bool(): pass
def test_hal_display_clear_accepts_color(): pass
def test_hal_display_draw_pixel_accepts_coordinates(): pass
def test_hal_display_flush_callable(): pass
def test_hal_display_typical_usage_sequence(): pass
def test_hal_display_multiple_draws_before_flush(): pass
def test_hal_display_clear_multiple_colors(): pass
def test_pixel_writes_mirror_to_shadow_framebuffer(): pass
def test_clear_fills_entire_shadow_framebuffer(): pass
def test_canvas_draw_updates_shadow_framebuffer(): pass
def test_rotation_swaps_width_and_height_at_90_degrees(): pass
def test_screenshot_dump_protocol_format(): pass
def test_hal_display_set_rotation_callable(): pass
def test_hal_display_rotation_90_swaps_dimensions(): pass
def test_hal_display_rotation_270_swaps_dimensions(): pass
def test_hal_display_rotation_0_180_no_swap(): pass
def test_hal_display_multiple_rotations(): pass
