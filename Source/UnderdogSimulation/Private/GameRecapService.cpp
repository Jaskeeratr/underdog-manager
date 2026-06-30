#include "GameRecapService.h"

namespace
{
    FString FormatClock(int32 Seconds)
    {
        return FString::Printf(TEXT("%d:%02d"), Seconds / 60, Seconds % 60);
    }

    FString FindPlayerName(const FMatchSnapshot& Snapshot, const FGuid& PlayerId)
    {
        for (const FPlayerProfile& P : Snapshot.HomeTeam.Players)
        {
            if (P.PlayerId == PlayerId) { return P.DisplayName; }
        }
        for (const FPlayerProfile& P : Snapshot.AwayTeam.Players)
        {
            if (P.PlayerId == PlayerId) { return P.DisplayName; }
        }
        return TEXT("Unknown");
    }

    FString TeamAbbrev(const FMatchSnapshot& Snapshot, const FGuid& TeamId)
    {
        if (TeamId == Snapshot.HomeTeam.TeamId) { return Snapshot.HomeTeam.Nickname; }
        if (TeamId == Snapshot.AwayTeam.TeamId) { return Snapshot.AwayTeam.Nickname; }
        return TEXT("???");
    }
}

FGameRecap FGameRecapService::BuildRecap(const FMatchResult& Result, const FMatchSnapshot& Snapshot)
{
    FGameRecap Recap;
    Recap.GameId = Result.GameId;
    Recap.HomeTeamId = Snapshot.HomeTeam.TeamId;
    Recap.AwayTeamId = Snapshot.AwayTeam.TeamId;
    Recap.HomeTeamName = Snapshot.HomeTeam.City + TEXT(" ") + Snapshot.HomeTeam.Nickname;
    Recap.AwayTeamName = Snapshot.AwayTeam.City + TEXT(" ") + Snapshot.AwayTeam.Nickname;
    Recap.FinalHomeScore = Result.HomeScore;
    Recap.FinalAwayScore = Result.AwayScore;

    Recap.QuarterScores.SetNum(Result.PeriodsPlayed);
    int32 RunningHome = 0;
    int32 RunningAway = 0;

    for (const FMatchEvent& Event : Result.Events)
    {
        const int32 PeriodIdx = Event.Period - 1;
        if (PeriodIdx >= 0 && PeriodIdx < Recap.QuarterScores.Num())
        {
            Recap.QuarterScores[PeriodIdx].HomePoints = Event.HomeScore - RunningHome;
            Recap.QuarterScores[PeriodIdx].AwayPoints = Event.AwayScore - RunningAway;
        }

        if (Event.Type == EMatchEventType::PeriodEnded && PeriodIdx >= 0 && PeriodIdx < Recap.QuarterScores.Num())
        {
            Recap.QuarterScores[PeriodIdx].HomePoints = Event.HomeScore - RunningHome;
            Recap.QuarterScores[PeriodIdx].AwayPoints = Event.AwayScore - RunningAway;
            RunningHome = Event.HomeScore;
            RunningAway = Event.AwayScore;
        }

        FString Desc;
        bool bHighlight = false;

        switch (Event.Type)
        {
        case EMatchEventType::ThreePointMade:
            Desc = FString::Printf(TEXT("%s drains a three! (%s)"),
                *FindPlayerName(Snapshot, Event.PrimaryPlayerId),
                *TeamAbbrev(Snapshot, Event.TeamId));
            bHighlight = true;
            break;
        case EMatchEventType::TwoPointMade:
            if (FMath::Abs(Event.HomeScore - Event.AwayScore) <= 3 && Event.ClockSeconds < 120)
            {
                Desc = FString::Printf(TEXT("%s scores a clutch bucket! (%s)"),
                    *FindPlayerName(Snapshot, Event.PrimaryPlayerId),
                    *TeamAbbrev(Snapshot, Event.TeamId));
                bHighlight = true;
            }
            break;
        case EMatchEventType::Block:
            Desc = FString::Printf(TEXT("%s with a big block on %s!"),
                *FindPlayerName(Snapshot, Event.PrimaryPlayerId),
                *FindPlayerName(Snapshot, Event.SecondaryPlayerId));
            bHighlight = true;
            break;
        case EMatchEventType::Steal:
            if (Event.ClockSeconds < 120)
            {
                Desc = FString::Printf(TEXT("%s with a crucial steal!"),
                    *FindPlayerName(Snapshot, Event.PrimaryPlayerId));
                bHighlight = true;
            }
            break;
        case EMatchEventType::PeriodEnded:
            if (Event.Period <= 4)
            {
                Desc = FString::Printf(TEXT("End of Q%d: %s %d - %s %d"),
                    Event.Period,
                    *Snapshot.HomeTeam.Nickname, Event.HomeScore,
                    *Snapshot.AwayTeam.Nickname, Event.AwayScore);
            }
            else
            {
                Desc = FString::Printf(TEXT("End of OT%d: %s %d - %s %d"),
                    Event.Period - 4,
                    *Snapshot.HomeTeam.Nickname, Event.HomeScore,
                    *Snapshot.AwayTeam.Nickname, Event.AwayScore);
            }
            bHighlight = true;
            break;
        case EMatchEventType::OvertimeStarted:
            Desc = FString::Printf(TEXT("We're heading to overtime! Tied at %d"), Event.HomeScore);
            bHighlight = true;
            break;
        case EMatchEventType::GameEnded:
        {
            const bool bHomeWon = Event.HomeScore > Event.AwayScore;
            Desc = FString::Printf(TEXT("FINAL: %s %d, %s %d"),
                bHomeWon ? *Snapshot.HomeTeam.Nickname : *Snapshot.AwayTeam.Nickname,
                bHomeWon ? Event.HomeScore : Event.AwayScore,
                bHomeWon ? *Snapshot.AwayTeam.Nickname : *Snapshot.HomeTeam.Nickname,
                bHomeWon ? Event.AwayScore : Event.HomeScore);
            bHighlight = true;
            break;
        }
        default:
            break;
        }

        if (!Desc.IsEmpty())
        {
            FPlayByPlayEntry Entry;
            Entry.Period = Event.Period;
            Entry.ClockSeconds = Event.ClockSeconds;
            Entry.Description = Desc;
            Entry.HomeScore = Event.HomeScore;
            Entry.AwayScore = Event.AwayScore;
            Entry.bHighlight = bHighlight;
            Recap.PlayByPlay.Add(Entry);
        }
    }

    // Also add top performer highlights from box score
    auto AddTopPerformer = [&](const TArray<FPlayerBoxScore>& BoxScore, const FString& TeamName)
    {
        const FPlayerBoxScore* Best = nullptr;
        for (const FPlayerBoxScore& Box : BoxScore)
        {
            if (!Best || Box.Points > Best->Points) { Best = &Box; }
        }
        if (Best && Best->Points >= 15)
        {
            FPlayByPlayEntry Entry;
            Entry.Period = Result.PeriodsPlayed;
            Entry.ClockSeconds = 0;
            Entry.Description = FString::Printf(TEXT("Top performer for %s: %s with %d pts, %d reb, %d ast"),
                *TeamName, *FindPlayerName(Snapshot, Best->PlayerId),
                Best->Points, Best->Rebounds, Best->Assists);
            Entry.HomeScore = Result.HomeScore;
            Entry.AwayScore = Result.AwayScore;
            Entry.bHighlight = true;
            Recap.PlayByPlay.Add(Entry);
        }
    };

    AddTopPerformer(Result.HomeBoxScore, Snapshot.HomeTeam.Nickname);
    AddTopPerformer(Result.AwayBoxScore, Snapshot.AwayTeam.Nickname);

    return Recap;
}
