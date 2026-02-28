"""
Traceability shim for theme_default C++ tests.

The real tests are C++ (test/test_theme_manager/) run via PlatformIO.
Results are recorded in tests.json from the PIO summary.
This shim exposes function names so the Critic traceability engine can match
Gherkin scenario keywords to test implementations via Python function discovery.
"""


def test_all_semantic_color_roles_defined(): pass   # Scenario: All semantic color roles are defined
def test_all_font_pointers_nonnull(): pass           # Scenario: All font pointers are non-null
def test_background_color_is_night(): pass          # Scenario: Background color is Night
