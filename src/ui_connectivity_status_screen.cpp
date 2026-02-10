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
        font = theme->fonts.heading;  // Use heading (24pt) instead of title (48pt)
    } else if (current_status == HAL_NETWORK_STATUS_ERROR) {
        message = "ERROR";
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

    // Calculate properly centered position using getTextBounds
    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();

    // Get text bounds to calculate proper centering
    int16_t x1, y1;
    uint16_t text_width, text_height;
    gfx->getTextBounds(message, 0, 0, &x1, &y1, &text_width, &text_height);

    // Calculate centered position
    int16_t text_x = (width - text_width) / 2;
    int16_t text_y = (height / 2) + (text_height / 2);  // Vertically centered baseline

    gfx->setCursor(text_x, text_y);
    gfx->print(message);

    // Note: Do NOT flush here - caller will handle flushing after rendering overlays (e.g., mini logo)
}
