#include "FlappyGame.h"
#include "../../Data/Profile/Achievements.h"
#include <algorithm>
#include <cmath>

namespace Flappy
{
    FlappyGame::FlappyGame(const sf::Font &font)
        : m_font(&font),
          m_accent(Accent::from(sf::Color(124, 255, 91))),
          m_jello(m_accent),
          m_pipes(),
          m_scene(),
          m_effects(font, m_accent),
          m_hud(font),
          m_ready(font),
          m_pause(font, m_accent),
          m_gameOver(font, m_accent),
          m_toast(font),
          m_view(sf::FloatRect({0.0f, 0.0f}, {kWidth, kHeight}))
    {
        start();
    }

    void FlappyGame::start()
    {
        applySettings();
        m_state = State::Ready;
        m_score = 0;
        m_streak = 0;
        m_newBest = false;
        m_gameOverTimer = 0.0f;
        m_overlayFade = 0.0f;
        m_readyFade = 1.0f;
        m_time = 0.0f;

        m_jello.reset();
        m_pipes.reset();
        m_pipes.setScrolling(true);
        m_effects.clear();

        m_hud.setScore(0);
        m_hud.setBest(m_best);
        m_hud.setStreak(0);
        m_hud.setPauseHovered(false);

        m_fanfareTimer = -1.0f;
        m_audio.setMusicDucked(false);
        m_audio.startMusic();
    }

    void FlappyGame::handleEvent(const sf::Event &event, sf::RenderWindow &window)
    {
        applyViewport(window);

        // Holding Space must not keep flapping
        window.setKeyRepeatEnabled(false);

        const auto *key = event.getIf<sf::Event::KeyPressed>();
        const auto *press = event.getIf<sf::Event::MouseButtonPressed>();
        const auto *release = event.getIf<sf::Event::MouseButtonReleased>();
        const auto *move = event.getIf<sf::Event::MouseMoved>();

        const bool flapKey = key && (key->code == sf::Keyboard::Key::Space ||
                                     key->code == sf::Keyboard::Key::Up ||
                                     key->code == sf::Keyboard::Key::W);
        const bool confirmKey = key && (key->code == sf::Keyboard::Key::Space ||
                                        key->code == sf::Keyboard::Key::Enter);
        const bool escKey = key && key->code == sf::Keyboard::Key::Escape;
        const bool pauseKey = key && key->code == sf::Keyboard::Key::P;

        // M mutes / unmutes everything, in any state
        if (key && key->code == sf::Keyboard::Key::M)
        {
            m_audio.toggleMute();
            UserData::instance().settings().muted = m_audio.isMuted();
            UserData::instance().commitSettings();
            return;
        }
        const bool leftPress = press && press->button == sf::Mouse::Button::Left;
        const bool leftRelease = release && release->button == sf::Mouse::Button::Left;

        auto runAction = [this](Action action)
        {
            if (action == Action::None)
            {
                return;
            }
            m_audio.playClick();
            switch (action)
            {
            case Action::Resume:
                resumeGame();
                break;
            case Action::Restart:
                restart();
                break;
            case Action::Menu:
                if (m_exitToMenu)
                {
                    exitToMenu();
                }
                break;
            case Action::None:
                break;
            }
        };

        switch (m_state)
        {
        case State::Ready:
            if (flapKey || leftPress)
            {
                beginPlaying();
            }
            else if (escKey && m_exitToMenu)
            {
                exitToMenu();
            }
            break;

        case State::Playing:
            if (flapKey)
            {
                flap();
            }
            else if (escKey || pauseKey)
            {
                pauseGame();
            }
            else if (leftPress)
            {
                if (m_hud.pauseHit(mouseToLogical(window, press->position)))
                {
                    pauseGame();
                }
                else
                {
                    flap();
                }
            }
            else if (move)
            {
                m_hud.setPauseHovered(m_hud.pauseHit(mouseToLogical(window, move->position)));
            }
            break;

        case State::Paused:
            if (escKey || pauseKey)
            {
                resumeGame();
            }
            else if (move)
            {
                m_pause.handleMouseMove(mouseToLogical(window, move->position));
            }
            else if (leftPress)
            {
                m_pause.handleMouseDown(mouseToLogical(window, press->position));
            }
            else if (leftRelease)
            {
                runAction(m_pause.handleMouseUp(mouseToLogical(window, release->position)));
            }
            break;

        case State::GameOver:
            if (confirmKey && m_gameOver.acceptsInput())
            {
                restart();
            }
            else if (escKey)
            {
                if (m_exitToMenu)
                {
                    exitToMenu();
                }
            }
            else if (move)
            {
                m_gameOver.handleMouseMove(mouseToLogical(window, move->position));
            }
            else if (leftPress)
            {
                m_gameOver.handleMouseDown(mouseToLogical(window, press->position));
            }
            else if (leftRelease)
            {
                runAction(m_gameOver.handleMouseUp(mouseToLogical(window, release->position)));
            }
            break;
        }
    }

    void FlappyGame::update(float dt)
    {
        dt = std::clamp(dt, 0.0f, 0.05f);
        m_time += dt;
        m_effects.update(dt);
        m_hud.update(dt);
        m_toast.update(dt);

        switch (m_state)
        {
        case State::Ready:
            m_scene.update(dt, m_pipes.speed() * 0.25f);
            m_jello.update(dt, false);
            m_ready.update(dt);
            m_readyFade = 1.0f;
            m_overlayFade = std::max(0.0f, m_overlayFade - 8.0f * dt);
            break;

        case State::Playing:
        {
            m_scene.update(dt, m_pipes.speed());
            m_jello.update(dt, true);
            m_pipes.update(dt);

            if (m_readyFade > 0.0f)
            {
                m_readyFade = std::max(0.0f, m_readyFade - 4.0f * dt);
                m_ready.update(dt);
            }
            m_overlayFade = std::max(0.0f, m_overlayFade - 8.0f * dt);

            m_effects.spawnTrail(m_jello.position(), dt);
            if (m_jello.consumeFlapEvent())
            {
                m_effects.spawnFlapBurst(m_jello.position());
            }

            // Scoring: each newly passed pipe adds a point; passing with the blob near the gap keeps the streak
            const int passed = m_pipes.collectScore(kPlayerX);
            for (int i = 0; i < passed; ++i)
            {
                ++m_score;
                const sf::Vector2f gap = m_pipes.lastScoredGapCenter();
                const bool perfect = std::abs(gap.y - m_jello.position().y) < 45.0f;
                m_streak = perfect ? m_streak + 1 : 0;
                m_effects.spawnScore(gap, 1);
                m_audio.playScore(m_streak);
                Achieve::onScore(0, m_score, m_difficulty, m_toast, m_audio);
                if (m_streak >= 10)
                {
                    Achieve::unlock(::Achievements::ComboKing, m_toast, m_audio);
                }
            }
            if (passed > 0)
            {
                m_hud.setScore(m_score);
                m_hud.setStreak(m_streak);
            }

            // Collision with pipes, the ground or flying off the top of the screen
            const sf::FloatRect box = m_jello.hitbox();
            const bool hitGround = box.position.y + box.size.y >= kGroundY;
            const bool offTop = m_jello.position().y < -40.0f;
            if (m_pipes.collides(box) || hitGround || offTop)
            {
                crash();
            }
            break;
        }

        case State::Paused:
            // World is frozen, only the overlay animates
            m_pause.update(dt);
            m_overlayFade = std::min(1.0f, m_overlayFade + 8.0f * dt);
            break;

        case State::GameOver:
            m_gameOverTimer += dt;
            if (m_fanfareTimer >= 0.0f)
            {
                m_fanfareTimer -= dt;
                if (m_fanfareTimer < 0.0f)
                {
                    m_audio.playNewBest();
                    m_fanfareTimer = -1.0f;
                }
            }
            m_scene.update(dt, 0.0f);
            m_jello.update(dt, false);
            if (m_gameOverTimer > 0.45f)
            {
                m_overlayFade = std::min(1.0f, m_overlayFade + 3.0f * dt);
            }
            m_gameOver.update(dt);
            break;
        }
    }

    void FlappyGame::draw(sf::RenderWindow &window)
    {
        applyViewport(window);
        window.clear(sf::Color::Black);

        sf::View view = m_view;
        view.setCenter(sf::Vector2f{kWidth * 0.5f, kHeight * 0.5f} + m_effects.shakeOffset());
        window.setView(view);

        m_scene.draw(window);
        m_pipes.draw(window);
        if (m_state == State::Ready || m_readyFade > 0.01f)
        {
            m_ready.draw(window, m_jello.position(), m_readyFade);
        }
        m_effects.drawBack(window);
        m_jello.drawShadow(window);
        m_jello.drawTrail(window);
        m_jello.draw(window);
        m_effects.drawFront(window);

        m_scene.drawVignette(window, 0.9f);

        if (m_state != State::GameOver)
        {
            m_hud.draw(window, m_state != State::Ready, 255);
        }

        if (m_state == State::Paused && m_overlayFade > 0.01f)
        {
            m_pause.draw(window, m_overlayFade);
        }
        if (m_state == State::GameOver && m_overlayFade > 0.01f)
        {
            m_gameOver.draw(window, m_overlayFade);
        }

        m_toast.draw(window);

        window.setView(window.getDefaultView());
    }

    void FlappyGame::applyViewport(const sf::RenderWindow &window)
    {
        const sf::Vector2u size = window.getSize();
        if (size.x == 0 || size.y == 0)
        {
            return;
        }

        const float winW = static_cast<float>(size.x);
        const float winH = static_cast<float>(size.y);
        const float aspect = kWidth / kHeight;

        // Largest 16:9 rectangle that fits, centered (letterbox)
        float w = winW;
        float h = winW / aspect;
        if (h > winH)
        {
            h = winH;
            w = winH * aspect;
        }

        const float fw = w / winW;
        const float fh = h / winH;
        m_view.setViewport(sf::FloatRect({(1.0f - fw) * 0.5f, (1.0f - fh) * 0.5f}, {fw, fh}));
    }

    sf::Vector2f FlappyGame::mouseToLogical(const sf::RenderWindow &window, sf::Vector2i pixel) const
    {
        return window.mapPixelToCoords(pixel, m_view);
    }

    void FlappyGame::beginPlaying()
    {
        m_state = State::Playing;
        flap();
    }

    void FlappyGame::flap()
    {
        m_jello.flap();
        m_audio.playFlap();
    }

    void FlappyGame::crash()
    {
        m_state = State::GameOver;
        m_gameOverTimer = 0.0f;
        m_overlayFade = 0.0f;
        m_jello.kill();
        m_pipes.setScrolling(false);
        m_effects.spawnExplosion(m_jello.position());
        m_audio.playCrash();
        m_audio.setMusicDucked(true);

        const int previousBest = m_best;
        m_newBest = m_score > m_best;
        if (m_newBest)
        {
            m_best = m_score;
            m_effects.spawnConfetti();
            m_fanfareTimer = 0.8f;
        }
        UserData::instance().recordGame(0, m_difficulty, m_score);
        Achieve::onRunEnd(m_toast, m_audio);
        m_hud.setBest(m_best);
        m_gameOver.show(m_score, previousBest, m_newBest);
    }

    void FlappyGame::pauseGame()
    {
        m_state = State::Paused;
        m_pause.setScores(m_score, m_best);
        m_audio.setMusicDucked(true);
        m_audio.playClick();
    }

    void FlappyGame::exitToMenu()
    {
        m_audio.stopMusic();
        if (m_exitToMenu)
        {
            m_exitToMenu();
        }
    }

    void FlappyGame::resumeGame()
    {
        m_state = State::Playing;
        m_audio.setMusicDucked(false);
    }

    void FlappyGame::restart()
    {
        start();
    }

    void FlappyGame::applySettings()
    {
        const UserData &data = UserData::instance();
        const Settings &settings = data.settings();

        m_difficulty = settings.difficulty;
        m_best = data.best(0, m_difficulty);

        m_accent = Accent::from(accentChoice(settings.accentIndex));
        m_jello.setAccent(m_accent);
        m_effects.setAccent(m_accent);
        m_pause = PauseOverlay(*m_font, m_accent);
        m_gameOver = GameOverOverlay(*m_font, m_accent);

        m_pipes.setDifficulty(m_difficulty);
        m_audio.setVolumes(settings.musicVolume, settings.sfxVolume, settings.muted);
        m_hud.setPlayerName(data.active().name);
    }
} // namespace Flappy
