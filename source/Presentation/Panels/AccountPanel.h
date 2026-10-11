#pragma once
#include "Panel.h"
#include "Widgets.h"
#include <string>
#include <vector>

namespace Panels
{
    // "Alt Manager": local player profiles. Card x 160..1120, y 40..680, title "PLAYERS".
    //
    // Left column (x 200..640): the profile list, one row per profile (row height 62, gap 8, starting y 150),
    //   showing the name, "Best <n>" for the current difficulty and a check badge on the active one.
    //   Clicking a row makes it active (UserData::setActive). Each row has a small round "x" delete button at its
    //   right end (UserData::deleteProfile; ignored when only one profile is left - draw it dimmed then).
    //   Below the list: "n / 6 players" counter.
    // Right column (x 680..1080): a stats card for the ACTIVE profile (name in big extruded text, games played,
    //   total score, best score for Easy / Normal / Hard), then a "New player" TextField (placeholder "Player name")
    //   with a primary "Add" button (Enter also submits) using UserData::addProfile; the error message returned
    //   (e.g. "That name is taken") shows in red below the field for ~3 seconds.
    //   The active profile's "Trophies n / 20" line sits at the top of the stats card.
    // Bottom: "Back" button (left) and "Trophies" button (right) that opens the TrophiesPanel.
    // Every change sets m_changed so the app can react (e.g. refresh the best score shown in the game).
    class AccountPanel : public Panel
    {
    public:
        explicit AccountPanel(const sf::Font &font);

        void onOpen() override;
        void update(float dt) override;
        void draw(sf::RenderTarget &target) const override;
        void handleMouseMove(sf::Vector2f point) override;
        void handleMouseDown(sf::Vector2f point) override;
        void handleMouseUp(sf::Vector2f point) override;
        void handleText(char32_t unicode) override;
        void handleKey(sf::Keyboard::Key key) override;

    private:
        const sf::Font *m_font;
        TextField m_nameField;
        std::vector<Flappy::UiKit::Button3D> m_buttons; // [0] Add  [1] Back  [2] Trophies
        int m_pressed = -1;
        int m_hoverRow = -1;
        int m_hoverDelete = -1;
        std::string m_error;
        float m_errorTimer = 0.0f;
        float m_time = 0.0f;
        Flappy::Accent accent() const;
        sf::FloatRect rowRect(int index) const;
        sf::FloatRect deleteRect(int index) const;
        void submitName();
    };
} // namespace Panels
