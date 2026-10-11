#include "InfoPanels.h"
#include "../../Data/Profile/UserData.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace Panels
{
    namespace
    {
        using Buttons = std::vector<Flappy::UiKit::Button3D>;

        const sf::FloatRect kHelpCard({190.0f, 40.0f}, {900.0f, 640.0f});
        const sf::FloatRect kHelpBack({490.0f, 600.0f}, {300.0f, 60.0f});
        const sf::FloatRect kCreditsCard({290.0f, 40.0f}, {700.0f, 640.0f});
        const sf::FloatRect kCreditsBack({490.0f, 608.0f}, {300.0f, 48.0f});

        // Help tab pills: six 132 x 40 pills with gaps of 8, centred under the title (832 px in total)
        constexpr int kTabCount = 6;
        constexpr float kTabTop = 155.0f;
        constexpr float kTabWidth = 132.0f;
        constexpr float kTabHeight = 40.0f;
        constexpr float kTabGap = 8.0f;
        constexpr float kTabsLeft = 640.0f - (kTabCount * kTabWidth + (kTabCount - 1) * kTabGap) / 2.0f;

        // Help content: the y values are block tops, every text line is centred on its own line
        constexpr float kContentTop = 240.0f;
        constexpr float kContentLeft = 250.0f;
        constexpr float kSecondColumn = 640.0f;
        constexpr float kHeadingStep = 34.0f;
        constexpr float kLineStep = 28.0f;
        constexpr float kControlStep = 40.0f;
        constexpr float kBlockGap = 10.0f;

        // Accent colour chosen in Settings
        Flappy::Accent currentAccent()
        {
            return Flappy::Accent::from(Flappy::accentChoice(UserData::instance().settings().accentIndex));
        }

        Buttons makeBackButton(const sf::Font &font, const Flappy::Accent &accent, sf::FloatRect rect)
        {
            Buttons buttons;
            buttons.emplace_back(font, std::string("Back"), rect, Flappy::UiKit::Button3DStyle::primary(accent), 30u);
            return buttons;
        }

        // Hover state of every button follows the cursor
        void hoverButtons(Buttons &buttons, sf::Vector2f point)
        {
            for (auto &button : buttons)
            {
                button.setHovered(button.contains(point));
            }
        }

        // Mouse down: the first button under the cursor gets the pressed look
        void pressButtons(Buttons &buttons, sf::Vector2f point, int &pressed)
        {
            for (std::size_t i = 0; i < buttons.size(); ++i)
            {
                if (buttons[i].contains(point))
                {
                    buttons[i].setPressed(true);
                    pressed = static_cast<int>(i);
                    return;
                }
            }
        }

        // Mouse up: returns true when the release happened over the pressed button, clears the pressed look
        bool releaseButtons(Buttons &buttons, sf::Vector2f point, int &pressed)
        {
            bool clicked = false;
            if (pressed >= 0 && static_cast<std::size_t>(pressed) < buttons.size())
            {
                clicked = buttons[static_cast<std::size_t>(pressed)].contains(point);
            }
            for (auto &button : buttons)
            {
                button.setPressed(false);
            }
            pressed = -1;
            return clicked;
        }

        // Filled disc centered on `center`
        void drawDisc(sf::RenderTarget &target, sf::Vector2f center, float radius, sf::Color color)
        {
            sf::CircleShape disc(radius);
            disc.setOrigin({radius, radius});
            disc.setPosition(center);
            disc.setFillColor(color);
            target.draw(disc);
        }

        // Key cap: white -> light blue face, 3 px border and a 4 px slab underneath. `cy` is the vertical center.
        // Returns the width of the cap so the action text can follow it.
        float drawKeyCap(sf::RenderTarget &target, const sf::Font &font, const char *keys, float x, float cy)
        {
            const float height = 34.0f;
            const float top = cy - height / 2.0f;
            const float width = std::max(96.0f, sf::Text(font, sf::String(keys), 20u).getLocalBounds().size.x + 32.0f);

            const sf::FloatRect slab({x, top + 4.0f}, {width, height});
            const sf::FloatRect border({x, top}, {width, height});
            const sf::FloatRect face({x + 3.0f, top + 3.0f}, {width - 6.0f, height - 6.0f});

            Flappy::UiKit::drawRoundedRect(target, slab, 10.0f, sf::Color(111, 134, 214));
            Flappy::UiKit::drawRoundedRect(target, border, 10.0f, sf::Color(122, 155, 214));
            Flappy::UiKit::drawRoundedRectGradient(target, face, 7.0f, sf::Color(255, 255, 255),
                                                   sf::Color(190, 216, 255));
            Flappy::UiKit::drawLabel(target, font, keys, 20, {x + width / 2.0f, cy}, sf::Color(20, 30, 80), false,
                                     1.0f);
            return width;
        }

        struct ControlRow
        {
            const char *keys;
            const char *action;
        };

        // Everything shown on one Help tab
        struct HelpTab
        {
            const char *name;
            std::vector<const char *> howToPlay;
            std::vector<ControlRow> controls;
            const char *extraLabel; // optional line under the controls (nullptr = none)
            const char *extraText;
            std::vector<const char *> tips;
        };

        const std::array<HelpTab, kTabCount> kHelpTabs{{
            {"General",
             {"Pick a game with PLAY on the main menu.",
              "Beat your best score; every profile keeps its own."},
             {{"ESC / P", "Pause"}, {"M", "Mute"}, {"F11", "Fullscreen"}, {"ENTER / SPACE", "Play again"}},
             nullptr,
             nullptr,
             {"Create profiles in Alt Manager and switch any time.",
              "Trophies unlock as you play - see them in Alt Manager.",
              "Difficulty and jello colour live in Settings."}},
            {"Bounce",
             {"Fly Jello through the gaps. Each pipe passed scores 1."},
             {{"SPACE / UP / W / CLICK", "Bounce"}},
             nullptr,
             nullptr,
             {"Small taps keep you steady.",
              "Fly through the middle of the gap for a streak.",
              "Hard mode: narrower gaps, faster world."}},
            {"Smash",
             {"Bounce the ball off your paddle and break every brick."},
             {{"MOUSE or A / D", "Move paddle"}, {"SPACE / CLICK", "Launch the ball"}},
             "Power-ups:",
             "W wide paddle, M multiball, S slow ball, L extra life",
             {"Red bomb bricks blow up their neighbours.",
              "Chain breaks without touching the paddle for a combo.",
              "Hit with the paddle edge to steer the ball."}},
            {"Blaster",
             {"Shoot asteroids and drones. You have three hearts."},
             {{"MOUSE or WASD", "Fly"}, {"HOLD SPACE or LEFT CLICK", "Fire"}},
             "Pickups:",
             "R rapid fire, T triple shot, S shield, H extra heart",
             {"Quick kills in a row build a combo multiplier.",
              "Big asteroids split into smaller ones.",
              "A mothership arrives every 40 kills."}},
            {"Slither",
             {"Eat fruit to grow. Do not hit yourself or a crystal."},
             {{"ARROWS or WASD", "Steer"}},
             nullptr,
             nullptr,
             {"Golden stars are worth 100 but vanish after 6 seconds.",
              "Easy mode lets you wrap around the walls.",
              "Eat fruit quickly to keep your combo."}},
            {"Stack",
             {"Drop the sliding block so it lands on the tower."},
             {{"SPACE or CLICK", "Drop the block"}},
             nullptr,
             nullptr,
             {"Overhang is cut away - the tower gets thinner.",
              "A PERFECT drop keeps the width; 3 in a row widen it.",
              "The block speeds up as the tower grows."}},
        }};

        struct CreditLine
        {
            const char *heading;
            const char *value;
        };

        const std::array<CreditLine, 6> kCredits{{
            {"DESIGN AND PROGRAMMING", "Nguyen Duong Gia Thuan"},
            {"BUILT WITH", "C++23  -  SFML 3.0  -  nlohmann/json"},
            {"SOUND AND MUSIC", "Synthesized in code, no samples used"},
            {"ART", "Every picture is drawn in code"},
            {"FONTS", "Geomanist, Corbel Light, Helvetica Neue UltraLight"},
            {"MADE WITH THE HELP OF", "Claude Code"},
        }};

        // Rectangle of tab pill `index`
        sf::FloatRect tabRect(int index)
        {
            return sf::FloatRect({kTabsLeft + static_cast<float>(index) * (kTabWidth + kTabGap), kTabTop},
                                 {kTabWidth, kTabHeight});
        }

        // Tab pill under `point`, -1 when none
        int tabAt(sf::Vector2f point)
        {
            for (int i = 0; i < kTabCount; ++i)
            {
                if (tabRect(i).contains(point))
                {
                    return i;
                }
            }
            return -1;
        }

        // One tab pill. The selected one is a candy button (gradient + dark slab), the others are dark glass.
        void drawTabPill(sf::RenderTarget &target, const sf::Font &font, const Flappy::Accent &a, int index,
                         const char *name, bool selected, bool hovered)
        {
            const sf::FloatRect rect = tabRect(index);
            const sf::Vector2f center{rect.position.x + rect.size.x / 2.0f, rect.position.y + rect.size.y / 2.0f};

            if (selected)
            {
                // Dark slab, dark 3 px border, light -> base gradient face
                const sf::FloatRect slab({rect.position.x, rect.position.y + 5.0f}, rect.size);
                Flappy::UiKit::drawRoundedRect(target, slab, 12.0f, a.dark);
                Flappy::UiKit::drawRoundedRect(target, rect, 12.0f, a.dark);
                const sf::FloatRect face({rect.position.x + 3.0f, rect.position.y + 3.0f},
                                         {rect.size.x - 6.0f, rect.size.y - 6.0f});
                const sf::Color top = hovered ? Flappy::mix(a.light, sf::Color::White, 0.35f) : a.light;
                Flappy::UiKit::drawRoundedRectGradient(target, face, 9.0f, top, a.base);
                Flappy::UiKit::drawLabel(target, font, name, 22, center, sf::Color(20, 30, 80), false, 1.0f);
            }
            else
            {
                const sf::Color fill = hovered ? sf::Color(40, 30, 110, 210) : sf::Color(14, 8, 60, 170);
                const sf::Color border(255, 255, 255, hovered ? 170 : 90);
                Flappy::UiKit::drawRoundedRect(target, rect, 12.0f, fill, border, 2.0f);
                Flappy::UiKit::drawLabel(target, font, name, 22, center, sf::Color::White, false, 1.0f);
            }
        }

        // Section heading in the accent colour; returns the top of the next block
        float drawHeading(sf::RenderTarget &target, const sf::Font &font, const char *title, float y, sf::Color color)
        {
            drawText(target, font, title, 26, {kContentLeft, y + 12.0f}, color);
            return y + kHeadingStep;
        }

        // One line of white body text
        float drawBodyLine(sf::RenderTarget &target, const sf::Font &font, const char *text, float y)
        {
            drawText(target, font, text, 22, {kContentLeft, y + 12.0f}, sf::Color::White);
            return y + kLineStep;
        }

        // Controls, filled row by row in two columns: key cap, then the action right after it
        float drawControlGrid(sf::RenderTarget &target, const sf::Font &font, const std::vector<ControlRow> &rows,
                              float y)
        {
            for (std::size_t i = 0; i < rows.size(); ++i)
            {
                const float x = (i % 2 == 0) ? kContentLeft : kSecondColumn;
                const float cy = y + 17.0f + kControlStep * static_cast<float>(i / 2);
                const float width = drawKeyCap(target, font, rows[i].keys, x, cy);
                drawText(target, font, rows[i].action, 22, {x + width + 14.0f, cy}, sf::Color::White);
            }
            const std::size_t rowCount = (rows.size() + 1) / 2;
            return y + kControlStep * static_cast<float>(rowCount);
        }

        // Label in the accent colour followed by the white list (power-ups, pickups)
        float drawExtraLine(sf::RenderTarget &target, const sf::Font &font, const char *label, const char *text,
                            float y, sf::Color labelColor)
        {
            drawText(target, font, label, 22, {kContentLeft, y + 12.0f}, labelColor);
            const float labelWidth = sf::Text(font, sf::String(label), 22u).getLocalBounds().size.x;
            drawText(target, font, text, 22, {kContentLeft + labelWidth + 8.0f, y + 12.0f}, sf::Color::White);
            return y + kLineStep;
        }

        // Tips with small accent bullets; returns the top of the next block
        float drawTips(sf::RenderTarget &target, const sf::Font &font, const std::vector<const char *> &tips, float y,
                       sf::Color bulletColor)
        {
            for (const char *tip : tips)
            {
                drawDisc(target, {kContentLeft + 8.0f, y + 12.0f}, 5.0f, bulletColor);
                drawText(target, font, tip, 22, {kContentLeft + 30.0f, y + 12.0f}, sf::Color::White);
                y += kLineStep;
            }
            return y;
        }
    } // namespace

    // ---------------- HelpPanel ----------------

    HelpPanel::HelpPanel(const sf::Font &font)
        : m_font(&font), m_buttons(makeBackButton(font, currentAccent(), kHelpBack))
    {
    }

    void HelpPanel::onOpen()
    {
        m_open = 0.0f;
        m_time = 0.0f;
        m_pressed = -1;
        m_closeRequested = false;
        m_tab = 0;
        m_hoverTab = -1;
        m_pressedTab = -1;
        m_buttons = makeBackButton(*m_font, accent(), kHelpBack); // picks up the current accent colour
    }

    void HelpPanel::update(float dt)
    {
        m_time += dt;
        m_open = std::min(1.0f, m_open + dt * 5.0f);
        for (auto &button : m_buttons)
        {
            button.update(dt);
        }
    }

    void HelpPanel::draw(sf::RenderTarget &target) const
    {
        drawCard(target, *m_font, kHelpCard, "HELP", m_open);
        if (m_open <= 0.3f)
        {
            return;
        }

        const Flappy::Accent a = accent();

        // Tab row
        for (int i = 0; i < kTabCount; ++i)
        {
            const char *name = kHelpTabs[static_cast<std::size_t>(i)].name;
            drawTabPill(target, *m_font, a, i, name, i == m_tab, i == m_hoverTab);
        }

        // Only the page of the selected tab is drawn
        const HelpTab &tab = kHelpTabs[static_cast<std::size_t>(m_tab)];
        float y = kContentTop;

        y = drawHeading(target, *m_font, "HOW TO PLAY", y, a.base);
        for (const char *line : tab.howToPlay)
        {
            y = drawBodyLine(target, *m_font, line, y);
        }
        y += kBlockGap;

        y = drawHeading(target, *m_font, "CONTROLS", y, a.base);
        y = drawControlGrid(target, *m_font, tab.controls, y);
        if (tab.extraLabel != nullptr && tab.extraText != nullptr)
        {
            y = drawExtraLine(target, *m_font, tab.extraLabel, tab.extraText, y, a.base);
        }
        y += kBlockGap;

        y = drawHeading(target, *m_font, "TIPS", y, a.base);
        drawTips(target, *m_font, tab.tips, y, a.base);

        for (const auto &button : m_buttons)
        {
            button.draw(target);
        }
    }

    void HelpPanel::handleMouseMove(sf::Vector2f point)
    {
        m_hoverTab = tabAt(point);
        hoverButtons(m_buttons, point);
    }

    void HelpPanel::handleMouseDown(sf::Vector2f point)
    {
        m_pressedTab = tabAt(point);
        pressButtons(m_buttons, point, m_pressed);
    }

    void HelpPanel::handleMouseUp(sf::Vector2f point)
    {
        // A tab switches when the release happens over the tab that was pressed
        if (m_pressedTab >= 0 && tabAt(point) == m_pressedTab)
        {
            m_tab = m_pressedTab;
        }
        m_pressedTab = -1;

        if (releaseButtons(m_buttons, point, m_pressed))
        {
            m_closeRequested = true;
        }
    }

    void HelpPanel::handleKey(sf::Keyboard::Key key)
    {
        if (key == sf::Keyboard::Key::Left)
        {
            m_tab = (m_tab + kTabCount - 1) % kTabCount;
        }
        else if (key == sf::Keyboard::Key::Right)
        {
            m_tab = (m_tab + 1) % kTabCount;
        }
    }

    Flappy::Accent HelpPanel::accent() const
    {
        return currentAccent();
    }

    // ---------------- CreditsPanel ----------------

    CreditsPanel::CreditsPanel(const sf::Font &font)
        : m_font(&font), m_buttons(makeBackButton(font, currentAccent(), kCreditsBack))
    {
    }

    void CreditsPanel::onOpen()
    {
        m_open = 0.0f;
        m_time = 0.0f;
        m_pressed = -1;
        m_closeRequested = false;
        m_buttons = makeBackButton(*m_font, accent(), kCreditsBack); // picks up the current accent colour
    }

    void CreditsPanel::update(float dt)
    {
        m_time += dt;
        m_open = std::min(1.0f, m_open + dt * 5.0f);
        for (auto &button : m_buttons)
        {
            button.update(dt);
        }
    }

    void CreditsPanel::draw(sf::RenderTarget &target) const
    {
        drawCard(target, *m_font, kCreditsCard, "CREDITS", m_open);
        if (m_open <= 0.3f)
        {
            return;
        }

        const Flappy::Accent a = accent();
        const sf::Color white = sf::Color::White;

        // Tiny jello logo that bobs up and down
        const float bob = std::sin(m_time * 2.0f) * 4.0f;
        const sf::Vector2f logo{509.0f, 200.0f + bob};
        Flappy::UiKit::drawShadedEllipse(target, logo, {34.0f, 30.0f}, a.light, a.base, a.dark);
        const sf::Color pupil(30, 20, 50);
        drawDisc(target, {logo.x - 11.0f, logo.y - 3.0f}, 6.5f, white);
        drawDisc(target, {logo.x + 11.0f, logo.y - 3.0f}, 6.5f, white);
        drawDisc(target, {logo.x - 9.5f, logo.y - 1.5f}, 3.3f, pupil);
        drawDisc(target, {logo.x + 12.5f, logo.y - 1.5f}, 3.3f, pupil);
        drawDisc(target, {logo.x - 15.0f, logo.y - 16.0f}, 5.0f, sf::Color(255, 255, 255, 200));

        // Game name
        Flappy::UiKit::drawExtrudedText(target, *m_font, "Jello Bounce", 44, {679.0f, 200.0f},
                                        Flappy::UiKit::orangeStyle());

        // Credit blocks with a gentle wave
        for (std::size_t i = 0; i < kCredits.size(); ++i)
        {
            const float index = static_cast<float>(i);
            const float y = 262.0f + 52.0f * index + std::sin(m_time * 1.5f + index) * 2.0f;
            Flappy::UiKit::drawLabel(target, *m_font, kCredits[i].heading, 20, {640.0f, y}, a.base, true, 3.0f);
            Flappy::UiKit::drawLabel(target, *m_font, kCredits[i].value, 26, {640.0f, y + 28.0f}, white, true, 1.0f);
        }

        Flappy::UiKit::drawLabel(target, *m_font, "Thanks for playing!", 28, {640.0f, 584.0f},
                                 sf::Color(255, 243, 176), true, 1.0f);

        for (const auto &button : m_buttons)
        {
            button.draw(target);
        }
    }

    void CreditsPanel::handleMouseMove(sf::Vector2f point)
    {
        hoverButtons(m_buttons, point);
    }

    void CreditsPanel::handleMouseDown(sf::Vector2f point)
    {
        pressButtons(m_buttons, point, m_pressed);
    }

    void CreditsPanel::handleMouseUp(sf::Vector2f point)
    {
        if (releaseButtons(m_buttons, point, m_pressed))
        {
            m_closeRequested = true;
        }
    }

    Flappy::Accent CreditsPanel::accent() const
    {
        return currentAccent();
    }
} // namespace Panels
