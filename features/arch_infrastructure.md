# Architectural Policy: Infrastructure

> Label: "Infrastructure Policy"
> Category: "ARCHITECTURES"
> Prerequisite: None

## 1. Configuration Management (The Secret Store)
*   **Zero-Secret Repository:** Credentials (Wi-Fi SSID, API Keys) **MUST NEVER** be committed to the repository.
*   **The `config.json` Bridge:** A `config.json` file in the project root serves as the local source of truth.
*   **Build-Time Injection:** PlatformIO scripts read `config.json` and inject values as C-preprocessor macros.
*   **Graceful Degradation:** If `config.json` is missing, the system MUST fallback to "Demo Mode" without crashing.

## 2. State Management
*   **The Ticker:** The `AnimationTicker` is the single source of truth for time.
*   **Frame Rate:** The system targets a stable 30fps.
*   **Delta Time:** All animations must be calculated based on `deltaTime` provided by the ticker, never on raw frame counts.
