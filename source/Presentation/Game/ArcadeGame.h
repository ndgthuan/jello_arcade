#pragma once
#include "FlappyCommon.h"
#include "MiniGame.h"
#include "Effects.h"
#include "Hud.h"
#include "ReadyOverlay.h"
#include "PauseOverlay.h"
#include "GameOverOverlay.h"
#include "GameAudio.h"
#include "Toast.h"
#include "../../Data/Profile/UserData.h"
#include <functional>
#include <string>

namespace Flappy
{
    // Base class of the arcade games (Smash, Blaster, Slither). It owns everything that is the same in every game:
    // the Ready -> Playing <-> Paused -> GameOver state machine, the HUD, the overlays, effects, audio, the
    // letterboxed 1280x720 view with screen shake, player profile best scores and the settings (accent colour,
    // difficulty, volumes). A game only implements the hooks below and draws its own world.
    //
    // Coordinates are logical (1280x720). The HUD occupies the top 130 px (score centered at y 95, pause button
    // top-left, best pill top-right, hearts at x 130..400 y 66): keep important play area below y = 130 unless the
    // game is fine with things passing under the HUD.
    class ArcadeGame : public IMiniGame
    {
    public:
        // gameId 0..3 (UserData best-score slot), musicTrack 0..3 (GameAudio::startMusic).
        ArcadeGame(const sf::Font &font, int gameId, int musicTrack);
        ~ArcadeGame() override = default;

        void setExitToMenuCallback(std::function<void()> callback) override { m_exitToMenu = std::move(callback); }
        void start() override;
        void handleEvent(const sf::Event &event, sf::RenderWindow &window) override;
        void update(float dt) override;
        void draw(sf::RenderWindow &window) override;

        State state() const { return m_state; }

    protected:
        // ------------------------------------------------------------------ hooks implemented by the game
        // Reset the whole world for a new run (called from start(), the difficulty is already known).
        virtual void onStart() = 0;
        // Advance the world. Called only while Playing.
        virtual void onUpdate(float dt) = 0;
        // Called while on the Ready or Game over screen (the world may keep animating, e.g. drifting stars).
        virtual void onIdle(float dt) { (void)dt; }
        // Backdrop (drawn first, before shake is applied? no: inside the shaken view) and the play field.
        virtual void drawBackground(sf::RenderTarget &target) const = 0;
        virtual void drawWorld(sf::RenderTarget &target) const = 0;

        // Texts of the Ready screen: the big title, a key cap label and the "to ..." action next to it
        virtual const char *readyTitle() const { return "GET READY!"; }
        virtual const char *readyKeyLabel() const { return "SPACE"; }
        virtual const char *readyActionText() const { return "to start!"; }
        // Which key / click leaves the Ready screen (default: Space, Enter, left click).
        virtual bool isStartKey(sf::Keyboard::Key key) const
        {
            return key == sf::Keyboard::Key::Space || key == sf::Keyboard::Key::Enter;
        }
        virtual bool startsOnClick() const { return true; }
        // Called once when the run begins (leaving Ready); a good place to launch the ball / first wave.
        virtual void onBegin() {}

        // Input while Playing (the base already handled Esc / P / M / the pause button).
        virtual void onKeyPressed(sf::Keyboard::Key key) { (void)key; }
        virtual void onKeyReleased(sf::Keyboard::Key key) { (void)key; }
        // Mouse position in logical coordinates; called for every move in Ready and Playing.
        virtual void onMouseMoved(sf::Vector2f point) { (void)point; }
        // Left / right press while Playing (a click on the pause button is consumed by the base).
        virtual void onMousePressed(sf::Vector2f point, sf::Mouse::Button button)
        {
            (void)point;
            (void)button;
        }
        virtual void onMouseReleased(sf::Vector2f point, sf::Mouse::Button button)
        {
            (void)point;
            (void)button;
        }

        // ------------------------------------------------------------------ services for the game
        // Adds points (also updates the HUD best pill when beaten, and the pop animation).
        void addScore(int points);
        int score() const { return m_score; }
        // Combo / streak shown as a pink tag when >= 2 (the word is set with setComboWord).
        void setCombo(int combo);
        void setComboWord(const std::string &word);
        // Hearts next to the pause button (0 hides them when maxLives == 0).
        void setLives(int lives, int maxLives);
        // Unlocks a trophy for the active profile (shows the toast the first time), e.g. Achievements::BossSlayer
        void unlockAchievement(int id);
        // Ends the run: explosion at `where` (scale 1 = the big crash explosion; < 1 uses a smaller blast),
        // crash sound, best score + profile stats saved, Game over overlay (with confetti on a new best).
        void gameOver(sf::Vector2f where, float scale = 1.0f);

        int difficulty() const { return m_difficulty; } // 0 easy, 1 normal, 2 hard
        const Accent &accent() const { return m_accent; }
        const sf::Font &font() const { return *m_font; }
        Effects &fx() { return m_effects; }
        GameAudio &audio() { return m_audio; }
        float runTime() const { return m_runTime; } // seconds since the run began (Playing time)
        float clockTime() const { return m_clock; } // seconds since start(), always advancing (for animation)
        sf::Vector2f mousePosition() const { return m_mouse; }
        // True while the key is held on the keyboard or the matching gamepad control (first pad only, see handleEvent)
        bool keyDown(sf::Keyboard::Key key) const;

    private:
        const sf::Font *m_font;
        int m_gameId;
        int m_musicTrack;
        Accent m_accent;
        Effects m_effects;
        Hud m_hud;
        ReadyOverlay m_ready;
        PauseOverlay m_pause;
        GameOverOverlay m_gameOver;
        GameAudio &m_audio = GameAudio::instance();
        Toast m_toast;

        State m_state = State::Ready;
        int m_score = 0;
        int m_best = 0;
        int m_difficulty = 1;
        bool m_newBest = false;
        float m_gameOverTimer = 0.0f;
        float m_overlayFade = 0.0f;
        float m_readyFade = 1.0f;
        float m_runTime = 0.0f;
        float m_clock = 0.0f;
        float m_fanfareTimer = -1.0f;
        sf::Vector2f m_mouse{kWidth * 0.5f, kHeight * 0.5f};
        sf::View m_view;
        std::function<void()> m_exitToMenu;

        // Gamepad state sampled in update(): sticks / POV past the dead zone, A button
        struct PadState
        {
            bool left = false;
            bool right = false;
            bool up = false;
            bool down = false;
            bool confirm = false;
        };
        PadState m_pad;

        void pollGamepad();
        void padKeyPressed(sf::Keyboard::Key key);
        void applyViewport(const sf::RenderWindow &window);
        sf::Vector2f mouseToLogical(const sf::RenderWindow &window, sf::Vector2i pixel) const;
        void beginPlaying();
        void pauseGame();
        void resumeGame();
        void exitToMenu();
        void applySettings();
    };
} // namespace Flappy
