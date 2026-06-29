#include "ChemistryService.h"

int32 FChemistryService::CalculateChemistry(const FTeamState& Team, const FLeagueState& League)
{
    int32 Score = 50;

    int32 TotalMorale = 0;
    for (const FAthleteState& State : Team.PlayerStates)
    {
        TotalMorale += State.Morale;
    }
    if (Team.PlayerStates.Num() > 0)
    {
        const int32 AvgMorale = TotalMorale / Team.PlayerStates.Num();
        Score += (AvgMorale - 50) / 3;
    }

    Score += FMath::Clamp(Team.WinStreak * 3, -15, 15);

    int32 MentorPairs = 0;
    for (const FMentorship& Pair : League.Mentorships)
    {
        const bool bVetOnTeam = Team.Players.ContainsByPredicate(
            [&Pair](const FPlayerProfile& P) { return P.PlayerId == Pair.VeteranId; });
        const bool bRookieOnTeam = Team.Players.ContainsByPredicate(
            [&Pair](const FPlayerProfile& P) { return P.PlayerId == Pair.RookieId; });
        if (bVetOnTeam && bRookieOnTeam) { MentorPairs++; }
    }
    Score += FMath::Min(MentorPairs * 4, 12);

    bool bHasPositions[5] = {};
    for (const FPlayerProfile& P : Team.Players)
    {
        bHasPositions[static_cast<int32>(P.Position)] = true;
    }
    int32 PositionsCovered = 0;
    for (bool b : bHasPositions) { PositionsCovered += b ? 1 : 0; }
    if (PositionsCovered >= 5) { Score += 5; }

    return FMath::Clamp(Score, 0, 100);
}

void FChemistryService::UpdateChemistryAfterGame(FTeamState& Team, bool bWon, const FLeagueState& League)
{
    if (bWon)
    {
        Team.WinStreak = FMath::Max(1, Team.WinStreak + 1);
    }
    else
    {
        Team.WinStreak = FMath::Min(-1, Team.WinStreak - 1);
    }
    Team.Chemistry = CalculateChemistry(Team, League);
}

void FChemistryService::UpdateChemistryAfterTrade(FTeamState& Team, int32 PlayersTraded)
{
    Team.Chemistry = FMath::Max(10, Team.Chemistry - PlayersTraded * 8);
}

void FChemistryService::UpdateMoraleAfterGame(FTeamState& Team, bool bWon, bool bCloseGame)
{
    for (FAthleteState& State : Team.PlayerStates)
    {
        if (bWon)
        {
            State.Morale = FMath::Min(100, State.Morale + (bCloseGame ? 4 : 2));
        }
        else
        {
            State.Morale = FMath::Max(5, State.Morale - (bCloseGame ? 2 : 4));
        }

        const int32 RotationIndex = Team.Rotation.OrderedPlayers.IndexOfByKey(State.PlayerId);
        if (RotationIndex != INDEX_NONE)
        {
            const int32 TargetMinutes = Team.Rotation.TargetMinutes.FindRef(State.PlayerId);
            if (TargetMinutes < 10 && State.Morale > 30)
            {
                State.Morale = FMath::Max(20, State.Morale - 2);
            }
        }
        else
        {
            State.Morale = FMath::Max(15, State.Morale - 3);
        }
    }
}

int32 FChemistryService::GetSimBonus(const FTeamState& Team)
{
    if (Team.Chemistry >= 80) { return 250; }
    if (Team.Chemistry >= 65) { return 120; }
    if (Team.Chemistry <= 25) { return -300; }
    if (Team.Chemistry <= 35) { return -150; }
    return 0;
}
