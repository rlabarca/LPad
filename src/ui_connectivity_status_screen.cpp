/**
 * @file ui_connectivity_status_screen.cpp
 * @brief Implementation of Connectivity Status Screen
 */

#include "ui_connectivity_status_screen.h"
#include "theme_manager.h"
#include "../hal/display.h"
#include <Arduino_GFX_Library.h>

using namespace LPad;

ConnectivityStatusScreen::ConnectivityStatusScreen()
    : m_display(nullptr)
    , m_last_status(HAL_NETWORK_STATUS_DISCONNECTED)
    , m_last_ping_result(false)
{
}

bool ConnectivityStatusScreen::begin(RelativeDisplay* display) {
    if (display == nullptr) {
        return false;
    }

    m_display = display;
    m_last_status = HAL_NETWORK_STATUS_DISCONNECTED;
    m_last_ping_result = false;

    return true;
}

void ConnectivityStatusScreen::update(bool ping_result) {
    if (m_display == nullptr) {
        return;
    }

    hal_network_status_t current_status = hal_network_get_status();

    // Only redraw if status or ping result changed
    if (current_status == m_last_status && ping_result == m_last_ping_result) {
        return;
    }

    m_last_status = current_status;
    m_last_ping_result = ping_result;

    // Get current theme
    const Theme* theme = ThemeManager::getInstance().getTheme();
    if (theme == nullptr) {
        return;
    }

    // Clear screen with theme background color
    m_display->drawSolidBackground(theme->colors.background);

    // Determine what to display
    const char* message = nullptr;
    const GFXfont* font = nullptr;

    if (current_status == HAL_NETWORK_STATUS_CONNECTING) {
        message = "CONNECTING...";
        font = theme->fonts.normal;
    } else if (current_status == HAL_NETWORK_STATUS_CONNECTED && ping_result) {
        message = "PING OK";
        font = theme->fonts.title;
    } else if (current_status == HAL_NETWORK_STATUS_ERROR) {
        message = "CONNECTIVITY ERROR";
        font = theme->fonts.normal;
    } else if (current_status == HAL_NETWORK_STATUS_CONNECTED && !ping_result) {
        message = "PING FAILED";
        font = theme->fonts.normal;
    } else {
        message = "DISCONNECTED";
        font = theme->fonts.normal;
    }

    // Get GFX object for direct text drawing
    Arduino_GFX* gfx = m_display->getGfx();
    if (gfx == nullptr) {
        return;
    }

    // Set font and color
    gfx->setFont(font);
    gfx->setTextColor(theme->colors.text_main);

    // Calculate centered position
    // For simplicity, use approximate center
    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();
    int32_t center_x = width / 2;
    int32_t center_y = height / 2;

    // Note: Arduino_GFX drawString doesn't support centering directly
    // Use setCursor and print for basic centering (approximate)
    gfx->setCursor(center_x - 60, center_y);  // Rough horizontal centering offset
    gfx->print(message);

    // Flush to display
    hal_display_flush();
}
