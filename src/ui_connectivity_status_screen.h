/**
 * @file ui_connectivity_status_screen.h
 * @brief Connectivity Status Screen
 *
 * Displays network connection status and ping test results.
 * See features/ui_connectivity_status_screen.md for complete specification.
 */

#ifndef UI_CONNECTIVITY_STATUS_SCREEN_H
#define UI_CONNECTIVITY_STATUS_SCREEN_H

#include "relative_display.h"
#include "../hal/network.h"

/**
 * @brief Connectivity Status Screen class
 *
 * Displays the current network connection status and the result of
 * the connectivity smoke test (ping).
 */
class ConnectivityStatusScreen {
public:
    /**
     * @brief Construct a new Connectivity Status Screen
     */
    ConnectivityStatusScreen();

    /**
     * @brief Initialize the screen
     *
     * @param display Pointer to the RelativeDisplay instance
     * @return true if initialization was successful
     */
    bool begin(RelativeDisplay* display);

    /**
     * @brief Update and render the screen
     *
     * Checks network status and displays appropriate message.
     *
     * @param ping_result Result of the ping test (true = success)
     */
    void update(bool ping_result);

private:
    RelativeDisplay* m_display;
    hal_network_status_t m_last_status;
    bool m_last_ping_result;
};

#endif // UI_CONNECTIVITY_STATUS_SCREEN_H
