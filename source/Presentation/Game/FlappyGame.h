#pragma once
#include "FlappyCommon.h"
#include "Jello.h"
#include "PipeField.h"
#include "Scene.h"
#include "Effects.h"
#include "Hud.h"
#include "ReadyOverlay.h"
#include "PauseOverlay.h"
#include "GameOverOverlay.h"
#include "GameAudio.h"
#include "MiniGame.h"
#include "Toast.h"
#include "../../Data/Profile/UserData.h"
#include <functional>
#include <string>

namespace Flappy
{
    // Jello Bounce: the whole game. Owns every part, runs the state machine
    // (Ready -> Playing <-> Paused -> GameOver -> Ready) and renders into a fixed 1280x720 logical view that is
    // letterboxed into the window.
    class FlappyGame : public IMiniGame
    {
    public:
        // `font` must outlive the game. Reads settings and the active profile from UserData.
        explicit FlappyGame(const sf::Font &font);

        // Called when the player chooses "Main menu" (pause or game over) or presses Esc on Ready/GameOver.
        void setExitToMenuCallback(std::function<void()> callback) override { m_exitToMenu = std::move(callback); }

        // Reset everything to the Ready state (call every time the game is opened from the menu).
        void start() override;

        // Input. Mouse positions are mapped to logical coordinates with window.mapPixelToCoords(pixel, m_view).
        //  Ready   : Space / Up / W / left click -> start playing with a first flap. Esc -> exit to menu.
        //  Playing : Space / Up / W / left click -> flap (left click on the pause button pauses instead);
        //            Esc / P -> pause.
        //  Paused  : Esc / P -> resume; mouse handled by PauseOverlay.
        //  GameOver: Space / Enter (once overlay.acceptsInput()) -> restart straight into Ready; Esc -> exit to menu;
        //            mouse handled by GameOverOverlay.
        void handleEvent(const sf::Event &event, sf::RenderWindow &window) override;

        void update(float dt) override;
        void draw(sf::RenderWindow &window) override;

        State state() const { return m_state; }

    private:
        const sf::Font *m_font;
        Accent m_accent;
        Jello m_jello;
        PipeField m_pipes;
        Scene m_scene;
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
        int m_streak = 0;
        bool m_newBest = false;
        float m_gameOverTimer = 0.0f; // time since the crash
        float m_overlayFade = 0.0f;   // fade of pause / game over overlays
        float m_readyFade = 1.0f;
        float m_time = 0.0f;
        float m_fanfareTimer = -1.0f; // counts down to the "new best" jingle after the crash sound
        sf::View m_view;
        std::function<void()> m_exitToMenu;

        void applyViewport(const sf::RenderWindow &window);
        sf::Vector2f mouseToLogical(const sf::RenderWindow &window, sf::Vector2i pixel) const;
        void beginPlaying();
        void flap();
        void crash();
        void exitToMenu();
        void pauseGame();
        void resumeGame();
        void restart();
        void applySettings(); // difficulty, colour, volumes and the active profile (called from start())
    };
} // namespace Flappy
