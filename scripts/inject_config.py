#!/usr/bin/env python3
"""
PlatformIO extra script to inject config.json values as build flags
"""
import json
import os

Import("env")

# Path to config.json relative to project root
config_path = os.path.join(env["PROJECT_DIR"], "config.json")

# Default values if config.json doesn't exist
wifi_ssid = "DEMO_MODE"
wifi_password = ""

# Try to read config.json
if os.path.exists(config_path):
    try:
        with open(config_path, 'r') as f:
            config = json.load(f)
            wifi_ssid = config.get("wifi_ssid", wifi_ssid)
            wifi_password = config.get("wifi_password", wifi_password)
        print(f"Loaded config from {config_path}")
    except Exception as e:
        print(f"Warning: Could not read config.json: {e}")
        print("Using default DEMO_MODE credentials")
else:
    print(f"Warning: {config_path} not found")
    print("Using default DEMO_MODE credentials")

# Inject as compiler flags
env.Append(CPPDEFINES=[
    ("LPAD_WIFI_SSID", env.StringifyMacro(wifi_ssid)),
    ("LPAD_WIFI_PASSWORD", env.StringifyMacro(wifi_password))
])

print(f"Injected LPAD_WIFI_SSID={wifi_ssid}")
