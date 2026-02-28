/**
 * @file data_item.h
 * @brief Base Data Model - Root Abstract Class for Data Objects
 *
 * This header defines the foundational abstract class `DataItem`, which serves as
 * the root for all data objects in the system. It establishes a uniform contract for
 * metadata (name, modification time) and memory management.
 *
 * See features/data_layer_core.md for complete specification.
 */

#ifndef DATA_ITEM_H
#define DATA_ITEM_H

#include "../../hal/timer.h"
#include <stdint.h>
#include <string>

/**
 * @class DataItem
 * @brief Abstract base class for all data objects in the system
 *
 * Provides uniform contract for metadata and memory management, allowing
 * higher-level components to interact with diverse data types polymorphically.
 */
class DataItem {
public:
    /**
     * @brief Constructs a DataItem with the given name
     * @param name The identifier for this data item
     */
    explicit DataItem(const std::string& name)
        : m_name(name), m_lastUpdated(0) {}

    /**
     * @brief Virtual destructor for polymorphic cleanup
     */
    virtual ~DataItem() = default;

    /**
     * @brief Gets the name/identifier of this data item
     * @return The data item name
     */
    const std::string& getName() const { return m_name; }

    /**
     * @brief Gets the timestamp of the last update
     * @return Timestamp in microseconds from hal_timer_get_micros()
     */
    uint64_t getLastUpdated() const { return m_lastUpdated; }

    /**
     * @brief Updates the timestamp to the current system time
     *
     * This method should be called whenever the data item's content is modified.
     */
    void touch() {
        m_lastUpdated = hal_timer_get_micros();
    }

protected:
    std::string m_name;         ///< Identifier for this data item
    uint64_t m_lastUpdated;     ///< Timestamp of last update (microseconds)
};

#endif // DATA_ITEM_H
