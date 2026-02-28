/**
 * @file mini_logo_component.h
 * @brief Mini Logo SystemComponent (Z=10)
 *
 * Wraps the existing MiniLogo as a passive overlay SystemComponent.
 * Always visible, transparent, draws logo to GFX buffer without flushing.
 */

#ifndef MINI_LOGO_COMPONENT_H
#define MINI_LOGO_COMPONENT_H

#include "../ui/ui_component.h"

class RelativeDisplay;
class MiniLogo;

class MiniLogoComponent : public SystemComponent {
public:
    MiniLogoComponent();
    ~MiniLogoComponent();

    bool begin(RelativeDisplay* display);

    void render() override;
    bool handleInput(const touch_gesture_event_t& event) override;

    bool isOpaque() const override { return false; }
    bool isFullscreen() const override { return false; }

private:
    MiniLogo* m_miniLogo;
};

#endif // MINI_LOGO_COMPONENT_H
