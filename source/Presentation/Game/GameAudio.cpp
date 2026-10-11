#include "GameAudio.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

namespace Flappy
{
    namespace
    {
        constexpr unsigned int kRate = 44100;
        constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;

        using Samples = std::vector<float>;

        float midiFreq(float midi)
        {
            return 440.0f * std::pow(2.0f, (midi - 69.0f) / 12.0f);
        }

        // One synthesized note: a sine with optional 2nd/3rd harmonics, a pitch sweep and an envelope
        // (linear attack, exponential decay, linear release at the end so it never clicks).
        struct Voice
        {
            float start = 0.0f;   // seconds from the start of the buffer
            float dur = 0.2f;     // length in seconds
            float freq = 440.0f;  // start frequency (Hz)
            float freqEnd = 0.0f; // end frequency (0 = same as freq)
            float amp = 0.3f;
            float attack = 0.005f;
            float decay = 8.0f;   // exponential decay rate (1/s), 0 = sustain
            float release = 0.01f;
            float h2 = 0.0f;
            float h3 = 0.0f;
        };

        void addVoice(Samples &buf, const Voice &v)
        {
            const auto first = static_cast<std::size_t>(std::max(0.0f, v.start) * kRate);
            const auto count = static_cast<std::size_t>(v.dur * kRate);
            const float endFreq = v.freqEnd > 0.0f ? v.freqEnd : v.freq;
            float phase = 0.0f;

            for (std::size_t i = 0; i < count; ++i)
            {
                const std::size_t index = first + i;
                if (index >= buf.size())
                {
                    break;
                }
                const float t = static_cast<float>(i) / kRate;
                const float k = static_cast<float>(i) / static_cast<float>(count);
                const float f = v.freq + (endFreq - v.freq) * k;
                phase += kTwoPi * f / kRate;

                float env = std::min(1.0f, t / std::max(v.attack, 0.0001f)) * std::exp(-v.decay * t);
                const float untilEnd = v.dur - t;
                if (untilEnd < v.release)
                {
                    env *= std::max(0.0f, untilEnd / v.release);
                }

                float s = std::sin(phase);
                if (v.h2 > 0.0f)
                {
                    s += v.h2 * std::sin(2.0f * phase);
                }
                if (v.h3 > 0.0f)
                {
                    s += v.h3 * std::sin(3.0f * phase);
                }
                buf[index] += s * env * v.amp;
            }
        }

        // Tiny deterministic noise source
        struct Noise
        {
            std::uint32_t state = 22222u;
            float next()
            {
                state = state * 1664525u + 1013904223u;
                return static_cast<float>((state >> 8) & 0xFFFFu) / 32768.0f - 1.0f;
            }
        };

        // Noise burst. `smooth` in (0,1]: small = dark rumble (low-passed), 1 = bright hiss.
        void addNoise(Samples &buf, Noise &rng, float start, float dur, float amp, float decay, float smooth)
        {
            const auto first = static_cast<std::size_t>(start * kRate);
            const auto count = static_cast<std::size_t>(dur * kRate);
            float low = 0.0f;
            for (std::size_t i = 0; i < count; ++i)
            {
                const std::size_t index = first + i;
                if (index >= buf.size())
                {
                    break;
                }
                const float t = static_cast<float>(i) / kRate;
                low += smooth * (rng.next() - low);
                const float untilEnd = dur - t;
                float env = std::exp(-decay * t);
                if (untilEnd < 0.01f)
                {
                    env *= std::max(0.0f, untilEnd / 0.01f);
                }
                buf[index] += low * env * amp;
            }
        }

        // High-passed noise (a hi-hat tick): noise minus its own low-passed version
        void addHat(Samples &buf, Noise &rng, float start, float amp)
        {
            const auto first = static_cast<std::size_t>(start * kRate);
            const auto count = static_cast<std::size_t>(0.05f * kRate);
            float low = 0.0f;
            for (std::size_t i = 0; i < count; ++i)
            {
                const std::size_t index = first + i;
                if (index >= buf.size())
                {
                    break;
                }
                const float t = static_cast<float>(i) / kRate;
                const float n = rng.next();
                low += 0.25f * (n - low);
                buf[index] += (n - low) * std::exp(-70.0f * t) * amp;
            }
        }

        bool toBuffer(const Samples &samples, sf::SoundBuffer &buffer, float gain = 1.0f)
        {
            std::vector<std::int16_t> pcm(samples.size());
            for (std::size_t i = 0; i < samples.size(); ++i)
            {
                // Gentle saturation keeps loud overlaps from clipping harshly
                const float y = std::tanh(1.3f * samples[i] * gain) / std::tanh(1.3f);
                pcm[i] = static_cast<std::int16_t>(std::clamp(y, -1.0f, 1.0f) * 0.85f * 32767.0f);
            }
            return buffer.loadFromSamples(pcm.data(), pcm.size(), 1, kRate, {sf::SoundChannel::Mono});
        }

        Samples makeSilence(float seconds)
        {
            return Samples(static_cast<std::size_t>(seconds * kRate), 0.0f);
        }

        // ---------------- effects ----------------

        void buildFlap(sf::SoundBuffer &out)
        {
            Samples s = makeSilence(0.2f);
            addVoice(s, {0.0f, 0.17f, 330.0f, 650.0f, 0.5f, 0.004f, 13.0f, 0.03f, 0.25f, 0.0f});
            addVoice(s, {0.0f, 0.12f, 660.0f, 1250.0f, 0.1f, 0.004f, 18.0f, 0.02f, 0.0f, 0.0f});
            (void)toBuffer(s, out);
        }

        void buildScore(sf::SoundBuffer &out)
        {
            Samples s = makeSilence(0.5f);
            addVoice(s, {0.0f, 0.16f, 988.0f, 0.0f, 0.4f, 0.004f, 12.0f, 0.03f, 0.3f, 0.0f});
            addVoice(s, {0.09f, 0.38f, 1319.0f, 0.0f, 0.4f, 0.004f, 8.0f, 0.06f, 0.3f, 0.1f});
            (void)toBuffer(s, out);
        }

        void buildCrash(sf::SoundBuffer &out)
        {
            Samples s = makeSilence(0.75f);
            Noise rng;
            addNoise(s, rng, 0.0f, 0.65f, 0.9f, 6.5f, 0.22f);
            addNoise(s, rng, 0.0f, 0.2f, 0.35f, 22.0f, 0.9f);
            addVoice(s, {0.0f, 0.45f, 150.0f, 42.0f, 0.9f, 0.002f, 6.0f, 0.05f, 0.2f, 0.0f});
            addVoice(s, {0.0f, 0.2f, 900.0f, 120.0f, 0.22f, 0.002f, 12.0f, 0.03f, 0.5f, 0.0f});
            (void)toBuffer(s, out);
        }

        void buildNewBest(sf::SoundBuffer &out)
        {
            Samples s = makeSilence(1.3f);
            const float notes[] = {72.0f, 76.0f, 79.0f, 84.0f, 88.0f};
            for (int i = 0; i < 5; ++i)
            {
                const float dur = (i == 4) ? 0.85f : 0.4f;
                addVoice(s, {0.1f * static_cast<float>(i), dur, midiFreq(notes[i]), 0.0f, 0.3f, 0.005f, 5.0f, 0.06f, 0.35f, 0.1f});
            }
            (void)toBuffer(s, out);
        }

        void buildClick(sf::SoundBuffer &out)
        {
            Samples s = makeSilence(0.1f);
            addVoice(s, {0.0f, 0.08f, 1300.0f, 900.0f, 0.35f, 0.002f, 28.0f, 0.02f, 0.2f, 0.0f});
            (void)toBuffer(s, out);
        }

        // ---------------- music ----------------

        struct Chord
        {
            float notes[3];
            float bass;
        };

        struct MusicSpec
        {
            float bpm;
            Chord prog[8];
            float melody[8][2]; // two bell notes per bar
            bool fourOnFloor;   // kick on every beat instead of beats 1 and 3
            bool bassPulse;     // eighth-note bass instead of two long notes
            bool busyHats;      // hi-hat on every eighth, louder
            float arpAmp;
            float padAmp;
        };

        // The four tracks: 0 Bounce (chill), 1 Smash (bright), 2 Blaster (driving), 3 Slither (mysterious)
        const MusicSpec &musicSpec(int track)
        {
            static const MusicSpec specs[6] = {
                {92.0f,
                 {{{57, 60, 64}, 45}, {{57, 60, 65}, 41}, {{55, 60, 64}, 48}, {{55, 59, 62}, 43},
                  {{57, 60, 64}, 45}, {{57, 62, 65}, 50}, {{57, 60, 65}, 41}, {{55, 59, 62}, 43}},
                 {{76, 79}, {77, 81}, {76, 72}, {74, 71}, {76, 79}, {77, 74}, {81, 77}, {79, 74}},
                 false, false, false, 0.10f, 0.045f},
                {124.0f,
                 {{{60, 64, 67}, 48}, {{55, 59, 62}, 43}, {{57, 60, 64}, 45}, {{53, 57, 60}, 41},
                  {{60, 64, 67}, 48}, {{55, 59, 62}, 43}, {{53, 57, 60}, 41}, {{55, 59, 62}, 43}},
                 {{79, 76}, {79, 74}, {81, 76}, {77, 72}, {79, 84}, {83, 79}, {77, 81}, {74, 79}},
                 true, false, true, 0.11f, 0.04f},
                {140.0f,
                 {{{55, 59, 64}, 40}, {{55, 60, 64}, 48}, {{55, 59, 62}, 43}, {{57, 62, 66}, 50},
                  {{55, 59, 64}, 40}, {{55, 60, 64}, 48}, {{57, 62, 66}, 50}, {{54, 59, 62}, 47}},
                 {{83, 79}, {84, 79}, {83, 78}, {81, 78}, {83, 79}, {84, 76}, {81, 78}, {83, 86}},
                 true, true, true, 0.13f, 0.035f},
                {100.0f,
                 {{{57, 62, 65}, 50}, {{58, 62, 65}, 46}, {{55, 58, 62}, 43}, {{57, 61, 64}, 45},
                  {{57, 62, 65}, 50}, {{58, 62, 65}, 46}, {{57, 61, 64}, 45}, {{57, 62, 65}, 50}},
                 {{74, 77}, {74, 70}, {70, 67}, {73, 69}, {74, 77}, {77, 74}, {73, 76}, {74, 69}},
                 false, false, false, 0.09f, 0.055f},
                // 4: Stack - cheerful, major pentatonic feel
                {110.0f,
                 {{{60, 64, 67}, 48}, {{57, 60, 64}, 45}, {{57, 60, 65}, 41}, {{55, 59, 62}, 43},
                  {{60, 64, 67}, 48}, {{55, 59, 64}, 40}, {{57, 60, 65}, 41}, {{55, 59, 62}, 43}},
                 {{79, 76}, {76, 72}, {77, 81}, {79, 74}, {79, 84}, {83, 79}, {81, 77}, {79, 74}},
                 false, false, true, 0.11f, 0.04f},
                // 5: menu - slow and dreamy
                {78.0f,
                 {{{57, 60, 65}, 41}, {{57, 60, 64}, 45}, {{57, 62, 65}, 50}, {{55, 60, 64}, 48},
                  {{57, 60, 65}, 41}, {{55, 59, 62}, 43}, {{57, 60, 64}, 45}, {{55, 60, 64}, 48}},
                 {{81, 77}, {79, 72}, {77, 74}, {76, 72}, {81, 84}, {83, 79}, {81, 77}, {79, 76}},
                 false, false, false, 0.08f, 0.06f}};
            return specs[track < 0 ? 0 : (track > 5 ? 5 : track)];
        }

        void buildMusic(sf::SoundBuffer &out, int track)
        {
            const MusicSpec &spec = musicSpec(track);
            constexpr int kBars = 8;
            const float beat = 60.0f / spec.bpm;
            const float bar = beat * 4.0f;
            const float eighth = beat * 0.5f;
            Samples s = makeSilence(bar * static_cast<float>(kBars));
            Noise rng;

            const int arpA[8] = {0, 1, 2, 1, 0, 1, 2, 1};
            const int arpB[8] = {0, 2, 1, 2, 0, 2, 1, 2};

            for (int b = 0; b < kBars; ++b)
            {
                const float t0 = bar * static_cast<float>(b);
                const Chord &ch = spec.prog[b];
                const bool last = (b == kBars - 1);

                // Pad: soft sustained triad that crossfades into the next bar
                for (float note : ch.notes)
                {
                    addVoice(s, {t0, bar + (last ? 0.0f : 0.3f), midiFreq(note), 0.0f, spec.padAmp, 0.45f, 0.0f, 0.35f, 0.0f, 0.0f});
                }

                // Bass
                if (spec.bassPulse)
                {
                    for (int e = 0; e < 8; ++e)
                    {
                        addVoice(s, {t0 + eighth * static_cast<float>(e), eighth * 0.9f, midiFreq(ch.bass), 0.0f, 0.22f, 0.008f, 6.0f, 0.03f, 0.3f, 0.0f});
                    }
                }
                else
                {
                    for (int k = 0; k < 2; ++k)
                    {
                        addVoice(s, {t0 + beat * 2.0f * static_cast<float>(k), beat * 1.6f, midiFreq(ch.bass), 0.0f, k == 0 ? 0.3f : 0.23f, 0.01f, 2.6f, 0.05f, 0.2f, 0.0f});
                    }
                }

                // Kick
                const int kicks = spec.fourOnFloor ? 4 : 2;
                for (int k = 0; k < kicks; ++k)
                {
                    const float tb = t0 + (spec.fourOnFloor ? beat * static_cast<float>(k) : beat * 2.0f * static_cast<float>(k));
                    addVoice(s, {tb, 0.2f, 120.0f, 45.0f, 0.32f, 0.002f, 17.0f, 0.02f, 0.0f, 0.0f});
                }

                // Hi-hat
                for (int e = spec.busyHats ? 0 : 1; e < 8; e += (spec.busyHats ? 1 : 2))
                {
                    addHat(s, rng, t0 + eighth * static_cast<float>(e), spec.busyHats ? 0.06f : 0.05f);
                }

                // Arpeggio pluck, one octave up
                const int *pattern = (b < 4) ? arpA : arpB;
                for (int e = 0; e < 8; ++e)
                {
                    addVoice(s, {t0 + eighth * static_cast<float>(e), 0.45f, midiFreq(ch.notes[pattern[e]] + 12.0f), 0.0f, spec.arpAmp, 0.003f, 7.0f, 0.04f, 0.35f, 0.12f});
                }

                // Bell melody with an echo
                for (int m = 0; m < 2; ++m)
                {
                    const float tm = t0 + beat * 2.0f * static_cast<float>(m);
                    addVoice(s, {tm, beat * 1.9f, midiFreq(spec.melody[b][m]), 0.0f, 0.075f, 0.01f, 3.0f, 0.1f, 0.3f, 0.0f});
                    addVoice(s, {tm + eighth * 1.5f, beat * 1.5f, midiFreq(spec.melody[b][m]), 0.0f, 0.026f, 0.01f, 3.5f, 0.1f, 0.3f, 0.0f});
                }
            }

            // Fade the very first and very last milliseconds so the loop point is click-free
            const std::size_t fade = static_cast<std::size_t>(0.02f * kRate);
            for (std::size_t i = 0; i < fade && i < s.size(); ++i)
            {
                const float g = static_cast<float>(i) / static_cast<float>(fade);
                s[i] *= g;
                s[s.size() - 1 - i] *= g;
            }
            (void)toBuffer(s, out, 1.0f);
        }

        // ---------------- extra effects for the other games ----------------

        void buildShoot(sf::SoundBuffer &out)
        {
            Samples s = makeSilence(0.14f);
            addVoice(s, {0.0f, 0.11f, 1500.0f, 320.0f, 0.3f, 0.002f, 22.0f, 0.02f, 0.5f, 0.0f});
            (void)toBuffer(s, out);
        }

        void buildPop(sf::SoundBuffer &out)
        {
            Samples s = makeSilence(0.16f);
            Noise rng;
            addVoice(s, {0.0f, 0.12f, 520.0f, 900.0f, 0.38f, 0.002f, 20.0f, 0.02f, 0.3f, 0.0f});
            addNoise(s, rng, 0.0f, 0.04f, 0.2f, 60.0f, 0.9f);
            (void)toBuffer(s, out);
        }

        void buildHit(sf::SoundBuffer &out)
        {
            Samples s = makeSilence(0.22f);
            addVoice(s, {0.0f, 0.18f, 240.0f, 100.0f, 0.55f, 0.002f, 16.0f, 0.03f, 0.4f, 0.0f});
            (void)toBuffer(s, out);
        }

        void buildPower(sf::SoundBuffer &out)
        {
            Samples s = makeSilence(0.5f);
            const float notes[] = {84.0f, 88.0f, 91.0f, 96.0f};
            for (int i = 0; i < 4; ++i)
            {
                addVoice(s, {0.06f * static_cast<float>(i), 0.3f, midiFreq(notes[i]), 0.0f, 0.28f, 0.004f, 8.0f, 0.04f, 0.3f, 0.1f});
            }
            (void)toBuffer(s, out);
        }

        void buildEat(sf::SoundBuffer &out)
        {
            Samples s = makeSilence(0.3f);
            addVoice(s, {0.0f, 0.1f, 700.0f, 1100.0f, 0.4f, 0.003f, 14.0f, 0.02f, 0.3f, 0.0f});
            addVoice(s, {0.06f, 0.18f, 1400.0f, 0.0f, 0.3f, 0.003f, 12.0f, 0.03f, 0.25f, 0.0f});
            (void)toBuffer(s, out);
        }

        void buildBlast(sf::SoundBuffer &out)
        {
            Samples s = makeSilence(0.45f);
            Noise rng;
            addNoise(s, rng, 0.0f, 0.4f, 0.8f, 11.0f, 0.3f);
            addVoice(s, {0.0f, 0.3f, 130.0f, 50.0f, 0.65f, 0.002f, 10.0f, 0.04f, 0.2f, 0.0f});
            (void)toBuffer(s, out);
        }
    } // namespace

    GameAudio::GameAudio()
    {
        buildFlap(m_flapBuffer);
        buildScore(m_scoreBuffer);
        buildCrash(m_crashBuffer);
        buildNewBest(m_bestBuffer);
        buildClick(m_clickBuffer);
        buildShoot(m_shootBuffer);
        buildPop(m_popBuffer);
        buildHit(m_hitBuffer);
        buildPower(m_powerBuffer);
        buildEat(m_eatBuffer);
        buildBlast(m_blastBuffer);
        buildMusic(m_musicBuffers[0], 0);
        m_musicBuilt[0] = true;

        m_flap.emplace(m_flapBuffer);
        m_score.emplace(m_scoreBuffer);
        m_crash.emplace(m_crashBuffer);
        m_best.emplace(m_bestBuffer);
        m_click.emplace(m_clickBuffer);
        for (int i = 0; i < 3; ++i)
        {
            m_shootPool.emplace_back(m_shootBuffer);
            m_popPool.emplace_back(m_popBuffer);
            m_blastPool.emplace_back(m_blastBuffer);
        }
        m_hit.emplace(m_hitBuffer);
        m_power.emplace(m_powerBuffer);
        m_eat.emplace(m_eatBuffer);
        m_music.emplace(m_musicBuffers[0]);
        m_music->setLooping(true);
        applyMusicVolume();
    }

    void GameAudio::playEffect(std::optional<sf::Sound> &sound, float volume, float pitch)
    {
        if (m_muted || !sound)
        {
            return;
        }
        sound->setVolume(volume * static_cast<float>(m_sfxVolume) / 100.0f);
        sound->setPitch(pitch);
        sound->play();
    }

    void GameAudio::playFlap()
    {
        playEffect(m_flap, 55.0f);
    }

    void GameAudio::playScore(int streak)
    {
        playEffect(m_score, 70.0f, 1.0f + 0.05f * static_cast<float>(std::clamp(streak, 0, 8)));
    }

    void GameAudio::playCrash()
    {
        playEffect(m_crash, 90.0f);
    }

    void GameAudio::playNewBest()
    {
        playEffect(m_best, 75.0f);
    }

    void GameAudio::playClick()
    {
        playEffect(m_click, 60.0f);
    }

    GameAudio &GameAudio::instance()
    {
        static GameAudio audio;
        return audio;
    }

    void GameAudio::playPooled(std::vector<sf::Sound> &pool, std::size_t &next, float volume, float pitch)
    {
        if (m_muted || pool.empty())
        {
            return;
        }
        sf::Sound &sound = pool[next % pool.size()];
        ++next;
        sound.setVolume(volume * static_cast<float>(m_sfxVolume) / 100.0f);
        sound.setPitch(pitch);
        sound.play();
    }

    void GameAudio::playShoot()
    {
        playPooled(m_shootPool, m_shootNext, 38.0f, 0.95f + 0.1f * static_cast<float>(m_shootNext % 3));
    }

    void GameAudio::playPop(float pitch)
    {
        playPooled(m_popPool, m_popNext, 65.0f, pitch);
    }

    void GameAudio::playHit()
    {
        playEffect(m_hit, 75.0f);
    }

    void GameAudio::playPower()
    {
        playEffect(m_power, 65.0f);
    }

    void GameAudio::playEat(float pitch)
    {
        playEffect(m_eat, 60.0f, pitch);
    }

    void GameAudio::playBlast()
    {
        playPooled(m_blastPool, m_blastNext, 70.0f, 0.9f + 0.2f * static_cast<float>(m_blastNext % 3) / 2.0f);
    }

    void GameAudio::startMusic(int track)
    {
        track = std::clamp(track, 0, 5);
        if (!m_music)
        {
            return;
        }
        const auto index = static_cast<std::size_t>(track);
        if (track != m_track)
        {
            // Switch tracks (the new one is synthesized the first time it is needed)
            m_music->stop();
            if (!m_musicBuilt[index])
            {
                buildMusic(m_musicBuffers[index], track);
                m_musicBuilt[index] = true;
            }
            m_music->setBuffer(m_musicBuffers[index]);
            m_music->setLooping(true);
            m_track = track;
        }
        if (m_music->getStatus() != sf::SoundSource::Status::Playing)
        {
            applyMusicVolume();
            m_music->play();
        }
    }

    void GameAudio::stopMusic()
    {
        if (m_music)
        {
            m_music->stop();
        }
    }

    void GameAudio::setMusicDucked(bool ducked)
    {
        m_ducked = ducked;
        applyMusicVolume();
    }

    void GameAudio::setVolumes(int music, int sfx, bool muted)
    {
        m_musicVolume = std::clamp(music, 0, 100);
        m_sfxVolume = std::clamp(sfx, 0, 100);
        m_muted = muted;
        applyMusicVolume();
    }

    void GameAudio::toggleMute()
    {
        m_muted = !m_muted;
        applyMusicVolume();
    }

    void GameAudio::applyMusicVolume()
    {
        if (m_music)
        {
            const float base = m_ducked ? 12.0f : 32.0f;
            m_music->setVolume(m_muted ? 0.0f : base * static_cast<float>(m_musicVolume) / 100.0f);
        }
    }
} // namespace Flappy
