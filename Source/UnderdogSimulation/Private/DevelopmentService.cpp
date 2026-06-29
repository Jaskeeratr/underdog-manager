#include "DevelopmentService.h"
#include "DeterministicRandom.h"

void FDevelopmentService::EstablishMentorships(FLeagueState& League)
{
    League.Mentorships.Reset();
    for (const FTeamState& Team : League.Teams)
    {
        for (const FPlayerProfile& Veteran : Team.Players)
        {
            if (Veteran.Age < 28 || Veteran.Ratings.Overall() < 65) { continue; }
            for (const FPlayerProfile& Rookie : Team.Players)
            {
                if (Rookie.PlayerId == Veteran.PlayerId) { continue; }
                if (Rookie.Age > 23) { continue; }
                if (Rookie.Position != Veteran.Position) { continue; }

                FMentorship Pair;
                Pair.VeteranId = Veteran.PlayerId;
                Pair.RookieId = Rookie.PlayerId;
                Pair.BonusRatingGain = FMath::Clamp((Veteran.Ratings.Overall() - 60) / 5, 1, 4);
                League.Mentorships.Add(Pair);
                break;
            }
        }
    }
}

void FDevelopmentService::ProcessDevelopment(FLeagueState& League)
{
    FDeterministicRandom Random(static_cast<uint64>(League.LeagueSeed)
        ^ (static_cast<uint64>(League.SeasonNumber) << 32) ^ 0x44455655LL);

    for (FTeamState& Team : League.Teams)
    {
        for (FPlayerProfile& Player : Team.Players)
        {
            if (Player.Age > 30) { continue; }

            const int32 GrowthPotential = Player.Ratings.Potential - Player.Ratings.Overall();
            if (GrowthPotential <= 0) { continue; }

            int32 GrowthChance = Player.Ratings.WorkEthic * 30 + GrowthPotential * 50;
            if (Player.Age <= 23) { GrowthChance += 2000; }
            else if (Player.Age <= 26) { GrowthChance += 1000; }

            const FMentorship* Mentor = League.Mentorships.FindByPredicate(
                [&Player](const FMentorship& M) { return M.RookieId == Player.PlayerId; });
            if (Mentor) { GrowthChance += 1500; }

            if (Random.ChancePerTenThousand(FMath::Clamp(GrowthChance, 1000, 8000)))
            {
                int32* Ratings[] = { &Player.Ratings.InsideScoring, &Player.Ratings.OutsideShooting,
                    &Player.Ratings.Playmaking, &Player.Ratings.PerimeterDefense,
                    &Player.Ratings.InteriorDefense, &Player.Ratings.Rebounding,
                    &Player.Ratings.Athleticism, &Player.Ratings.Stamina };
                int32* Target = Ratings[Random.Range(0, 7)];
                int32 Gain = Random.Range(1, 2);
                if (Mentor) { Gain += Mentor->BonusRatingGain > 2 ? 1 : 0; }
                *Target = FMath::Min(99, *Target + Gain);
                Player.Ratings.Clamp();

                FAthleteState* State = Team.PlayerStates.FindByPredicate(
                    [&Player](const FAthleteState& S) { return S.PlayerId == Player.PlayerId; });
                if (State) { State->SeasonDevelopment += Gain; }
            }
        }
    }
}

void FDevelopmentService::ApplyBreakoutBust(FLeagueState& League)
{
    FDeterministicRandom Random(static_cast<uint64>(League.LeagueSeed)
        ^ (static_cast<uint64>(League.SeasonNumber) << 24) ^ 0x42524B4F5554ULL);

    for (FTeamState& Team : League.Teams)
    {
        for (FPlayerProfile& Player : Team.Players)
        {
            if (Player.Age < 20 || Player.Age > 25) { continue; }

            // Breakout: high potential, high work ethic, young
            if (Player.Ratings.Potential >= 75 && Player.Ratings.WorkEthic >= 70
                && Random.ChancePerTenThousand(800))
            {
                int32* Ratings[] = { &Player.Ratings.InsideScoring, &Player.Ratings.OutsideShooting,
                    &Player.Ratings.Playmaking, &Player.Ratings.PerimeterDefense,
                    &Player.Ratings.InteriorDefense, &Player.Ratings.Rebounding };
                for (int32 Boosts = 0; Boosts < 3; ++Boosts)
                {
                    int32* Target = Ratings[Random.Range(0, 5)];
                    *Target = FMath::Min(99, *Target + Random.Range(3, 6));
                }
                Player.Ratings.Clamp();
            }

            // Bust: low work ethic, low potential
            if (Player.Ratings.WorkEthic <= 35 && Player.Ratings.Potential <= 55
                && Random.ChancePerTenThousand(600))
            {
                int32* Ratings[] = { &Player.Ratings.InsideScoring, &Player.Ratings.OutsideShooting,
                    &Player.Ratings.Playmaking, &Player.Ratings.PerimeterDefense,
                    &Player.Ratings.InteriorDefense, &Player.Ratings.Rebounding };
                for (int32 Drops = 0; Drops < 2; ++Drops)
                {
                    int32* Target = Ratings[Random.Range(0, 5)];
                    *Target = FMath::Max(1, *Target - Random.Range(3, 5));
                }
                Player.Ratings.Clamp();
            }
        }
    }
}
