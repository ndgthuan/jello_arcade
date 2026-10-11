#pragma once
#include "Panel.h"
#include "Widgets.h"
#include <vector>

namespace Panels
{
    // Settings: Music volume, Effects volume, Mute, Jello colour, Difficulty, Fullscreen button, Back button.
    // Card: x 240..1040, y 40..680, title "SETTINGS". Reads / writes UserData::instance().settings() and calls
    // commitSettings() after every change, and sets m_changed so the app can re-apply volumes.
    class SettingsPanel : public Panel
    {
    public:
        explicit SettingsPanel(const sf::Font &font);

        void onOpen() override;
        void update(float dt) override;
        void draw(sf::RenderTarget &target) const override;
        void handleMouseMove(sf::Vector2f point) override;
        void handleMouseDown(sf::Vector2f point) override;
        void handleMouseUp(sf::Vector2f point) override;

    private:
        const sf::Font *m_font;
        Slider m_music;
        Slider m_sfx;
        Toggle m_mute;
        SwatchRow m_color;
        Segmented m_difficulty;
        Toggle m_shake;
        std::vector<Flappy::UiKit::Button3D> m_buttons; // [0] Fullscreen  [1] Back
        int m_pressed = -1;
        float m_time = 0.0f;
        Flappy::Accent accent() const;
    };
} // namespace Panels
