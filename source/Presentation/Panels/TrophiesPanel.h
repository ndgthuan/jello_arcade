#pragma once
#include "Panel.h"
#include "Widgets.h"
#include <vector>

namespace Panels
{
    // Trophies of the ACTIVE profile (UserData::instance(), Achievements::info / Count in
    // Data/Profile/Achievements.h). Card x 130..1150, y 40..680, title "TROPHIES".
    // Under the title: a chip "<profile name>  -  <unlocked> / <Count>" and a progress bar.
    // Then a grid of tiles, 4 columns x 5 rows (tile 232 x 82, gap 12, starting x 168, y 190), in id order:
    // each tile has a round medal on the left (gold, shaded, with a star, when unlocked; flat grey with a small
    // padlock when locked), the trophy name (white / dim) and the description under it (small, wrapped to two
    // lines when longer than ~26 characters). Unlocked tiles are brighter and have a gold border; hovering a tile
    // lifts it slightly. Bottom: "Back" button (primary style).
    class TrophiesPanel : public Panel
    {
    public:
        explicit TrophiesPanel(const sf::Font &font);

        void onOpen() override;
        void update(float dt) override;
        void draw(sf::RenderTarget &target) const override;
        void handleMouseMove(sf::Vector2f point) override;
        void handleMouseDown(sf::Vector2f point) override;
        void handleMouseUp(sf::Vector2f point) override;

    private:
        const sf::Font *m_font;
        std::vector<Flappy::UiKit::Button3D> m_buttons; // [0] Back
        int m_pressed = -1;
        int m_hover = -1;
        float m_time = 0.0f;
        Flappy::Accent accent() const;
        sf::FloatRect tileRect(int index) const;
    };
} // namespace Panels
