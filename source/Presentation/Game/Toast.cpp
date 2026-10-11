#include "Toast.h"
#include "UiKit.h"
#include "../../Data/Profile/Achievements.h"
#include "../../Data/Profile/UserData.h"
#include <algorithm>
#include <cmath>

namespace Flappy
{
    namespace
    {
        constexpr float kSlideIn = 0.4f;
        constexpr float kHold = 3.0f;
        constexpr float kSlideOut = 0.4f;
        constexpr float kWidth_ = 560.0f;
        constexpr float kHeight_ = 84.0f;
        constexpr float kRestY = 592.0f; // top edge when fully shown

        float easeOutBack(float t)
        {
            const float c1 = 1.70158f;
            const float c3 = c1 + 1.0f;
            const float u = t - 1.0f;
            return 1.0f + c3 * u * u * u + c1 * u * u;
        }
    } // namespace

    void Toast::show(const std::string &heading, const std::string &name)
    {
        m_queue.push_back(Item{heading, name});
    }

    void Toast::update(float dt)
    {
        if (!m_showing)
        {
            if (m_queue.empty())
            {
                return;
            }
            m_current = m_queue.front();
            m_queue.pop_front();
            m_showing = true;
            m_age = 0.0f;
        }

        m_age += dt;
        if (m_age >= kSlideIn + kHold + kSlideOut)
        {
            m_showing = false;
        }
    }

    void Toast::draw(sf::RenderTarget &target) const
    {
        if (!m_showing)
        {
            return;
        }

        // Slide from below the screen to the rest position and back
        float slide = 1.0f;
        if (m_age < kSlideIn)
        {
            slide = easeOutBack(m_age / kSlideIn);
        }
        else if (m_age > kSlideIn + kHold)
        {
            const float t = (m_age - kSlideIn - kHold) / kSlideOut;
            slide = 1.0f - t * t;
        }
        const float x = (kWidth - kWidth_) * 0.5f;
        const float y = kHeight + (kRestY - kHeight) * slide;

        // Soft glow, slab, face
        UiKit::drawGlow(target, {kWidth * 0.5f, y + kHeight_ * 0.5f}, 360.0f, sf::Color(255, 200, 80, 70));
        UiKit::drawRoundedRect(target, sf::FloatRect({x, y + 8.0f}, {kWidth_, kHeight_}), 26.0f, sf::Color(70, 40, 10, 230));
        UiKit::drawRoundedRectGradient(target, sf::FloatRect({x, y}, {kWidth_, kHeight_}), 26.0f,
                                       sf::Color(255, 226, 122), sf::Color(255, 176, 32));
        UiKit::drawRoundedRect(target, sf::FloatRect({x + 6.0f, y + 4.0f}, {kWidth_ - 12.0f, 4.0f}), 2.0f,
                               sf::Color(255, 255, 255, 170));

        // Medal
        const sf::Vector2f medal{x + 54.0f, y + kHeight_ * 0.5f};
        UiKit::drawShadedEllipse(target, medal, {30.0f, 30.0f}, sf::Color(255, 251, 208), sf::Color(255, 194, 71),
                                 sf::Color(168, 90, 6));
        UiKit::drawRing(target, medal, 24.0f, 3.0f, sf::Color(255, 243, 176, 220));
        UiKit::drawSparkle(target, medal, 15.0f, sf::Color(90, 42, 0), 0.0f);
        const float twinkle = 0.5f + 0.5f * std::sin(m_age * 10.0f);
        UiKit::drawSparkle(target, medal + sf::Vector2f{22.0f, -22.0f}, 9.0f * (0.6f + 0.4f * twinkle),
                           sf::Color(255, 255, 255, 230), 0.0f);

        // Text (left aligned next to the medal)
        const auto drawLeft = [&](const std::string &text, unsigned int size, sf::Vector2f pos, sf::Color color)
        {
            sf::Text t(*m_font, text, size);
            const sf::FloatRect b = t.getLocalBounds();
            t.setOrigin(sf::Vector2f{b.position.x, b.position.y + b.size.y / 2.0f});
            t.setPosition(pos);
            t.setFillColor(color);
            t.setLetterSpacing(1.0f);
            target.draw(t);
        };
        drawLeft(m_current.heading, 15, {x + 106.0f, y + 26.0f}, sf::Color(120, 60, 0));
        drawLeft(m_current.name, 32, {x + 106.0f, y + 56.0f}, sf::Color(70, 30, 0));
    }

    namespace Achieve
    {
        void unlock(int id, Toast &toast, GameAudio &audio)
        {
            if (UserData::instance().unlockAchievement(id))
            {
                toast.show("ACHIEVEMENT UNLOCKED", ::Achievements::info(id).name);
                audio.playPower();
            }
        }

        void onScore(int game, int score, int difficulty, Toast &toast, GameAudio &audio)
        {
            if (game < 0 || game >= kGameCount)
            {
                return;
            }
            for (int tier = 0; tier < 3; ++tier)
            {
                if (score >= ::Achievements::threshold(game, tier))
                {
                    unlock(game * 3 + tier, toast, audio);
                }
            }
            if (difficulty == 2 && score >= ::Achievements::threshold(game, 1))
            {
                unlock(::Achievements::HardHero, toast, audio);
            }
        }

        void onRunEnd(Toast &toast, GameAudio &audio)
        {
            const UserData &data = UserData::instance();
            const Profile &p = data.active();
            bool all = true;
            for (int g = 0; g < kGameCount; ++g)
            {
                all = all && p.plays[static_cast<std::size_t>(g)] > 0;
            }
            if (all)
            {
                unlock(::Achievements::Explorer, toast, audio);
            }
            if (p.gamesPlayed >= 25)
            {
                unlock(::Achievements::Veteran, toast, audio);
            }
        }
    } // namespace Achieve
} // namespace Flappy
