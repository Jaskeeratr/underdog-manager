#include "RivalryService.h"

namespace
{
    FRivalry& FindOrCreateRivalry(FLeagueState& League, const FGuid& TeamA, const FGuid& TeamB)
    {
        const FGuid& First = TeamA < TeamB ? TeamA : TeamB;
        const FGuid& Second = TeamA < TeamB ? TeamB : TeamA;

        for (FRivalry& R : League.Rivalries)
        {
            if (R.TeamAId == First && R.TeamBId == Second) { return R; }
        }

        FRivalry NewRivalry;
        NewRivalry.TeamAId = First;
        NewRivalry.TeamBId = Second;
        return League.Rivalries.Add_GetRef(NewRivalry);
    }
}

void FRivalryService::UpdateAfterGame(FLeagueState& League, const FGuid& HomeTeamId,
    const FGuid& AwayTeamId, int32 ScoreDiff, bool bPlayoffs)
{
    FRivalry& Rivalry = FindOrCreateRivalry(League, HomeTeamId, AwayTeamId);

    if (bPlayoffs)
    {
        Rivalry.PlayoffMeetings++;
        Rivalry.Intensity += 15;
    }

    if (FMath::Abs(ScoreDiff) <= 5)
    {
        Rivalry.CloseGames++;
        Rivalry.Intensity += 5;
    }
    else if (FMath::Abs(ScoreDiff) <= 10)
    {
        Rivalry.Intensity += 2;
    }

    Rivalry.Intensity = FMath::Min(Rivalry.Intensity, 100);
}

void FRivalryService::DecayRivalries(FLeagueState& League)
{
    for (int32 Index = League.Rivalries.Num() - 1; Index >= 0; --Index)
    {
        League.Rivalries[Index].Intensity = FMath::Max(0, League.Rivalries[Index].Intensity - 5);
        if (League.Rivalries[Index].Intensity <= 0)
        {
            League.Rivalries.RemoveAt(Index);
        }
    }
}

const FRivalry* FRivalryService::GetRivalry(const FLeagueState& League, const FGuid& TeamA, const FGuid& TeamB)
{
    const FGuid& First = TeamA < TeamB ? TeamA : TeamB;
    const FGuid& Second = TeamA < TeamB ? TeamB : TeamA;

    return League.Rivalries.FindByPredicate(
        [&](const FRivalry& R) { return R.TeamAId == First && R.TeamBId == Second; });
}

int32 FRivalryService::GetMoraleBonus(const FLeagueState& League, const FGuid& TeamId, const FGuid& OpponentId)
{
    const FRivalry* Rivalry = GetRivalry(League, TeamId, OpponentId);
    if (!Rivalry) { return 0; }
    if (Rivalry->Intensity >= 60) { return 5; }
    if (Rivalry->Intensity >= 30) { return 2; }
    return 0;
}

TArray<FRivalry> FRivalryService::GetTopRivalries(const FLeagueState& League, int32 Count)
{
    TArray<FRivalry> Sorted = League.Rivalries;
    Sorted.Sort([](const FRivalry& A, const FRivalry& B) { return A.Intensity > B.Intensity; });
    if (Sorted.Num() > Count) { Sorted.SetNum(Count); }
    return Sorted;
}
