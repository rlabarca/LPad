# Feature: HAL Timer Specification

> Label: "HAL Timer Specification"
> Category: "Hardware Layer"
> Prerequisite: features/hal_core_contract.md

## Introduction

This document defines the abstract interface for high-resolution timing operations within the Hardware Abstraction Layer (HAL).

## Timer HAL API

### `hal_timer_init(void)`
*   **Description:** Initializes the high-resolution hardware timer.
*   **Returns:** `bool` - `true` if success.

### `hal_timer_get_micros(void)`
*   **Description:** Returns microseconds since boot or init.
*   **Returns:** `uint64_t` monotonic value.
