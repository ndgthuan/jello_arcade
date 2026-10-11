#pragma once

// The trophies a player can earn. Each profile has its own set (a bit mask in UserData).
namespace Achievements
{
    enum Id : int
    {
        // Score tiers per game (game order: Bounce, Smash, Blaster, Slither, Stack)
        Bounce1, Bounce2, Bounce3,
        Smash1, Smash2, Smash3,
        Blaster1, Blaster2, Blaster3,
        Slither1, Slither2, Slither3,
        Stack1, Stack2, Stack3,
        // Specials
        ComboKing,  // a combo / streak of 10 in any game
        BossSlayer, // destroy the Blaster mothership
        Explorer,   // play every game at least once
        HardHero,   // reach the second score tier of any game on Hard
        Veteran,    // finish 25 runs
        Count
    };

    struct Info
    {
        const char *name;
        const char *description;
        int game; // game id the trophy belongs to, or -1 for specials
    };

    inline const Info &info(int id)
    {
        static const Info kInfo[Count] = {
            {"First Flight", "Score 5 in Jello Bounce", 0},
            {"Pipe Dreamer", "Score 20 in Jello Bounce", 0},
            {"Sky Legend", "Score 50 in Jello Bounce", 0},
            {"Brick Rookie", "Score 100 in Jello Smash", 1},
            {"Brick Breaker", "Score 500 in Jello Smash", 1},
            {"Candy Crusher", "Score 1500 in Jello Smash", 1},
            {"Cadet", "Score 300 in Jello Blaster", 2},
            {"Ace Pilot", "Score 1500 in Jello Blaster", 2},
            {"Star Commander", "Score 5000 in Jello Blaster", 2},
            {"Little Worm", "Score 50 in Jello Slither", 3},
            {"Long Snake", "Score 200 in Jello Slither", 3},
            {"Mega Serpent", "Score 500 in Jello Slither", 3},
            {"First Floors", "Stack 10 blocks in Jello Stack", 4},
            {"Skyscraper", "Stack 30 blocks in Jello Stack", 4},
            {"Tower of Jello", "Stack 60 blocks in Jello Stack", 4},
            {"Combo King", "Reach a combo of 10 in any game", -1},
            {"Boss Slayer", "Destroy the Blaster mothership", -1},
            {"Explorer", "Play all five games", -1},
            {"Hard Hero", "Reach the second tier of any game on Hard", -1},
            {"Veteran", "Finish 25 runs", -1}};
        return kInfo[id < 0 ? 0 : (id >= Count ? Count - 1 : id)];
    }

    // Score needed for tier 0..2 of a game (0..4); the tier trophy id is game * 3 + tier
    inline int threshold(int game, int tier)
    {
        static const int kThresholds[5][3] = {{5, 20, 50}, {100, 500, 1500}, {300, 1500, 5000}, {50, 200, 500}, {10, 30, 60}};
        const int g = game < 0 ? 0 : (game > 4 ? 4 : game);
        const int t = tier < 0 ? 0 : (tier > 2 ? 2 : tier);
        return kThresholds[g][t];
    }
} // namespace Achievements
