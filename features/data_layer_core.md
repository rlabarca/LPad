# Data Layer: Base Data Model

> Label: "Base Data Model"
> Category: "Data Layer"
> Prerequisite: features/hal_core_contract.md

## Description
This feature defines the foundational abstract class `DataItem`, which serves as the root for all data objects in the system. It establishes a uniform contract for metadata (name, modification time) and memory management, ensuring that higher-level components (like the UI or Persistence Layer) can interact with diverse data types polymorphically.

## Constraints
*   **Object-Oriented:** Must be implemented as a C++ Abstract Base Class (or one with virtual methods) to allow polymorphism.
*   **Decoupled:** Must NOT depend on UI or high-level application logic. It should only depend on core types and the HAL Timer for timestamps.
*   **Memory Management:** Must declare a `virtual` destructor to ensure proper cleanup of derived classes.

## Scenarios

### Scenario 1: Base Identity
GIVEN a concrete subclass of `DataItem` is instantiated with the name "StockPrice"
WHEN I inspect the `name` property
THEN it should be "StockPrice"

### Scenario 2: Timestamp Tracking
GIVEN a `DataItem` instance
WHEN I call the method to update its timestamp (e.g., `touch()` or `setLastUpdated()`)
THEN the `lastUpdated` field should be set to the current system time from `hal_timer_get_time()`

### Scenario 3: Polymorphic Cleanup
GIVEN a pointer of type `DataItem*` pointing to a derived class that allocates dynamic memory
WHEN I `delete` the `DataItem*` pointer
THEN the derived class's destructor should be invoked
AND all dynamic memory should be freed correctly
