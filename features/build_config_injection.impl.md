# Implementation Notes: Build Config Injection

### Source Mapping

| File | Role |
|---|---|
| `scripts/inject_config.py` | PlatformIO pre-build hook |
| `config.example.json` | Template for the gitignored `config.json` |
| `src/wifi_config_generated.h` | Generated output (committed) |
| `tests/build_config_injection/test_config_injection.py` | Python unit tests |

### PlatformIO SCons API Constraint

The script uses `Import("env")` (SCons API) and cannot be executed standalone. Tests bypass this by importing the module's helper functions directly after patching `builtins.__import__` to mock the SCons `Import()` call.

### [AUTONOMOUS] Idempotent Write via Content Comparison

The spec requires idempotent write. Implemented by reading the existing file (if present) and comparing against the new content string before writing. Uses `os.path.exists()` + file read; no hash or mtime comparison. This prevents PlatformIO from triggering a full rebuild on every pre-script invocation.

### [AUTONOMOUS] DEMO_MODE Credentials Are Empty String

The spec says `("DEMO_MODE", "")`. The implementation uses an empty password string, not a null pointer, to avoid potential undefined behavior in WiFi libraries that may not handle null credentials.

### Special Character Escaping

Backslash and double-quote are the only characters escaped. Forward slashes and other special characters in SSIDs/passwords are passed through verbatim (valid in C string literals).
