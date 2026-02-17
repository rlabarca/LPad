/**
 * @file boot_logo_app.h
 * @brief Boot Logo Application Component (Z=5)
 *
 * Plays a full-screen logo animation on initial boot, then transitions
 * to the main application. Does NOT run on resume from suspend.
 *
 * Specification: features/app_boot_logo.md
 */

#ifndef BOOT_LOGO_APP_H
#define BOOT_LOGO_APP_H

#include "../ui/ui_component.h"

class RelativeDisplay;
class UIRenderManager;

class BootLogoApp : public AppComponent {
public:
    enum class AnimState { WAIT, ANIMATE, DONE };

    BootLogoApp();
    ~BootLogoApp();

    /**
     * Initialize the boot logo app.
     * @param display RelativeDisplay instance for rendering
     * @param nextApp AppComponent to transition to after animation completes
     * @param miniLogo SystemComponent to show when animation completes (optional)
     */
    bool begin(RelativeDisplay* display, AppComponent* nextApp,
               SystemComponent* miniLogo = nullptr);

    // UIComponent lifecycle
    void onRun() override;
    void onClose() override {}
    void render() override;
    void update(float dt) override;
    bool handleInput(const touch_gesture_event_t& event) override;

    bool isOpaque() const override { return true; }
    bool isFullscreen() const override { return true; }

private:
    RelativeDisplay* m_display;
    AppComponent* m_nextApp;
    SystemComponent* m_miniLogo;

    AnimState m_state;
    float m_timer;
    bool m_needsRender;
    bool m_transitioned;

    // Current interpolated animation parameters
    float m_x_percent;
    float m_y_percent;
    float m_width_percent;
    float m_anchor_x;
    float m_anchor_y;

    // Timing
    static constexpr float WAIT_DURATION = 2.0f;
    static constexpr float ANIMATE_DURATION = 1.5f;

    // Start state: centered, 75% of screen height
    static constexpr float START_HEIGHT_PERCENT = 75.0f;
    static constexpr float START_X_PERCENT = 50.0f;
    static constexpr float START_Y_PERCENT = 50.0f;
    static constexpr float START_ANCHOR_X = 0.5f;
    static constexpr float START_ANCHOR_Y = 0.5f;

    // End state: top-right corner, 10% of screen height, 10px buffer
    static constexpr float END_HEIGHT_PERCENT = 10.0f;
    static constexpr float CORNER_OFFSET_PX = 10.0f;

    static float easeInOutCubic(float t);
    float heightToWidthPercent(float heightPercent) const;
    void calculateEndPosition(float& x, float& y) const;
};

#endif // BOOT_LOGO_APP_H
