#!/usr/bin/env python3
"""
PlatformIO extra script to inject config.json values as build flags.
Supports multi-WiFi configuration per features/sys_config_system.md.

Config format (multi-WiFi):
{
    "wifi": [
        {"ssid": "Network1", "password": "Pass1"},
        {"ssid": "Network2", "password": "Pass2"}
    ]
}

Backward-compatible format (single WiFi):
{
    "wifi_ssid": "Network1",
    "wifi_password": "Pass1"
}

Generates:
    -DLPAD_WIFI_SSID="first_ssid"
    -DLPAD_WIFI_PASSWORD="first_password"
    -DLPAD_WIFI_COUNT=N
    -DLPAD_WIFI_CONFIG={"ssid1","pass1"},{"ssid2","pass2"}
"""
import json
import os

Import("env")

config_path = os.path.join(env["PROJECT_DIR"], "config.json")

wifi_entries = []

if os.path.exists(config_path):
    try:
        with open(config_path, 'r') as f:
            config = json.load(f)

        # New multi-WiFi format
        if "wifi" in config and isinstance(config["wifi"], list):
            for entry in config["wifi"]:
                ssid = entry.get("ssid", "")
                password = entry.get("password", "")
                if ssid:
                    wifi_entries.append((ssid, password))
            print(f"Loaded {len(wifi_entries)} WiFi entries from config.json")

        # Legacy single-WiFi format
        elif "wifi_ssid" in config:
            ssid = config.get("wifi_ssid", "DEMO_MODE")
            password = config.get("wifi_password", "")
            wifi_entries.append((ssid, password))
            print(f"Loaded 1 WiFi entry from config.json (legacy format)")

        else:
            print("Warning: config.json has no wifi entries")

    except Exception as e:
        print(f"Warning: Could not read config.json: {e}")
else:
    print(f"Warning: {config_path} not found")
    print("Using default DEMO_MODE credentials")

# Fallback: no config at all
if not wifi_entries:
    wifi_entries.append(("DEMO_MODE", ""))

# Build the LPAD_WIFI_CONFIG macro: {"ssid","pass"},{"ssid2","pass2"}
config_parts = []
for ssid, password in wifi_entries:
    config_parts.append('{"%s","%s"}' % (ssid.replace('"', '\\"'),
                                          password.replace('"', '\\"')))
config_value = ",".join(config_parts)

# Inject build flags
env.Append(CPPDEFINES=[
    ("LPAD_WIFI_SSID", env.StringifyMacro(wifi_entries[0][0])),
    ("LPAD_WIFI_PASSWORD", env.StringifyMacro(wifi_entries[0][1])),
    ("LPAD_WIFI_COUNT", str(len(wifi_entries))),
])

# LPAD_WIFI_CONFIG is injected as a raw flag (not stringified)
env.Append(CPPFLAGS=[
    '-DLPAD_WIFI_CONFIG=%s' % config_value
])

print(f"Injected LPAD_WIFI_SSID={wifi_entries[0][0]}")
print(f"Injected LPAD_WIFI_COUNT={len(wifi_entries)}")
