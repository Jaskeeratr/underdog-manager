#include "CommentaryService.h"

namespace
{
    const FPlayerProfile* FindPlayer(const FMatchSnapshot& Snapshot, const FGuid& PlayerId)
    {
        const FPlayerProfile* Found = Snapshot.HomeTeam.Players.FindByPredicate(
            [&PlayerId](const FPlayerProfile& P) { return P.PlayerId == PlayerId; });
        if (!Found)
        {
            Found = Snapshot.AwayTeam.Players.FindByPredicate(
                [&PlayerId](const FPlayerProfile& P) { return P.PlayerId == PlayerId; });
        }
        return Found;
    }

    FString TeamName(const FMatchSnapshot& Snapshot, const FGuid& TeamId)
    {
        if (TeamId == Snapshot.HomeTeam.TeamId) { return Snapshot.HomeTeam.Nickname; }
        if (TeamId == Snapshot.AwayTeam.TeamId) { return Snapshot.AwayTeam.Nickname; }
        return TEXT("Unknown");
    }

    FString PlayerName(const FMatchSnapshot& Snapshot, const FGuid& PlayerId)
    {
        const FPlayerProfile* Player = FindPlayer(Snapshot, PlayerId);
        return Player ? Player->DisplayName : TEXT("Unknown");
    }

    FString ClockStr(int32 ClockSeconds)
    {
        const int32 Minutes = ClockSeconds / 60;
        const int32 Seconds = ClockSeconds % 60;
        return FString::Printf(TEXT("%d:%02d"), Minutes, Seconds);
    }

    int32 ScoreDiff(const FMatchEvent& Event, const FGuid& ScoringTeamId, const FMatchSnapshot& Snapshot)
    {
        if (ScoringTeamId == Snapshot.HomeTeam.TeamId) { return Event.HomeScore - Event.AwayScore; }
        return Event.AwayScore - Event.HomeScore;
    }
}

TArray<FCommentaryLine> FCommentaryService::Generate(const FMatchResult& Result, const FMatchSnapshot& Snapshot)
{
    TArray<FCommentaryLine> Lines;
    TMap<FGuid, int32> RunningPoints;

    for (const FMatchEvent& Event : Result.Events)
    {
        FCommentaryLine Line;
        Line.Period = Event.Period;
        Line.ClockSeconds = Event.ClockSeconds;
        Line.Importance = 0;

        const FString PName = PlayerName(Snapshot, Event.PrimaryPlayerId);
        const FString TName = TeamName(Snapshot, Event.TeamId);
        const bool bClutch = Event.ClockSeconds <= 120 && Event.Period >= 4;
        const int32 Margin = FMath::Abs(Event.HomeScore - Event.AwayScore);

        switch (Event.Type)
        {
        case EMatchEventType::GameStarted:
            Line.Text = FString::Printf(TEXT("Tip-off! %s host the %s."),
                *Snapshot.HomeTeam.Nickname, *Snapshot.AwayTeam.Nickname);
            Line.Importance = 3;
            break;

        case EMatchEventType::TwoPointMade:
        {
            RunningPoints.FindOrAdd(Event.PrimaryPlayerId) += 2;
            const int32 PlayerPts = RunningPoints[Event.PrimaryPlayerId];
            if (bClutch && Margin <= 3)
            {
                Line.Text = FString::Printf(TEXT("CLUTCH! %s scores inside to make it %d-%d!"),
                    *PName, Event.HomeScore, Event.AwayScore);
                Line.Importance = 8;
            }
            else if (PlayerPts >= 30)
            {
                Line.Text = FString::Printf(TEXT("%s scores again! %d points and counting for the %s star."),
                    *PName, PlayerPts, *TName);
                Line.Importance = 5;
            }
            else if (Event.SecondaryPlayerId.IsValid())
            {
                Line.Text = FString::Printf(TEXT("%s finds %s for the bucket. %s %d-%d."),
                    *PlayerName(Snapshot, Event.SecondaryPlayerId), *PName,
                    *ClockStr(Event.ClockSeconds), Event.HomeScore, Event.AwayScore);
                Line.Importance = 1;
            }
            else
            {
                Line.Text = FString::Printf(TEXT("%s scores inside. %d-%d."),
                    *PName, Event.HomeScore, Event.AwayScore);
                Line.Importance = 1;
            }
            break;
        }
        case EMatchEventType::ThreePointMade:
        {
            RunningPoints.FindOrAdd(Event.PrimaryPlayerId) += 3;
            const int32 PlayerPts = RunningPoints[Event.PrimaryPlayerId];
            if (bClutch && Margin <= 3)
            {
                Line.Text = FString::Printf(TEXT("FROM DOWNTOWN! %s drains the clutch three! %d-%d!"),
                    *PName, Event.HomeScore, Event.AwayScore);
                Line.Importance = 9;
            }
            else if (PlayerPts >= 25)
            {
                Line.Text = FString::Printf(TEXT("%s buries another three! That's %d points tonight!"),
                    *PName, PlayerPts);
                Line.Importance = 6;
            }
            else
            {
                Line.Text = FString::Printf(TEXT("%s knocks down the three-pointer for %s. %d-%d."),
                    *PName, *TName, Event.HomeScore, Event.AwayScore);
                Line.Importance = 2;
            }
            break;
        }
        case EMatchEventType::TwoPointMissed:
        case EMatchEventType::ThreePointMissed:
            continue;

        case EMatchEventType::FreeThrowMade:
            RunningPoints.FindOrAdd(Event.PrimaryPlayerId) += 1;
            Line.Text = FString::Printf(TEXT("%s sinks the free throw. %d-%d."),
                *PName, Event.HomeScore, Event.AwayScore);
            Line.Importance = bClutch && Margin <= 2 ? 6 : 0;
            break;

        case EMatchEventType::FreeThrowMissed:
            if (bClutch && Margin <= 3)
            {
                Line.Text = FString::Printf(TEXT("%s misses at the line! The pressure gets to him."), *PName);
                Line.Importance = 5;
            }
            else { continue; }
            break;

        case EMatchEventType::Steal:
            Line.Text = FString::Printf(TEXT("%s with the steal for %s!"), *PName, *TName);
            Line.Importance = bClutch ? 5 : 2;
            break;

        case EMatchEventType::Block:
            Line.Text = FString::Printf(TEXT("%s rejects it! Big block by %s!"), *PName, *TName);
            Line.Importance = bClutch ? 6 : 3;
            break;

        case EMatchEventType::Turnover:
            if (bClutch && Margin <= 5)
            {
                Line.Text = FString::Printf(TEXT("Costly turnover by %s! %s give it away in crunch time."),
                    *PName, *TName);
                Line.Importance = 5;
            }
            else { continue; }
            break;

        case EMatchEventType::OffensiveRebound:
            Line.Text = FString::Printf(TEXT("%s grabs the offensive board. Second chance for %s."),
                *PName, *TName);
            Line.Importance = 1;
            break;

        case EMatchEventType::DefensiveRebound:
            continue;

        case EMatchEventType::Foul:
            continue;

        case EMatchEventType::PeriodEnded:
            Line.Text = FString::Printf(TEXT("End of Q%d. Score: %s %d, %s %d."),
                Event.Period, *Snapshot.HomeTeam.Nickname, Event.HomeScore,
                *Snapshot.AwayTeam.Nickname, Event.AwayScore);
            Line.Importance = 4;
            break;

        case EMatchEventType::OvertimeStarted:
            Line.Text = TEXT("We're headed to overtime! Neither team can be separated!");
            Line.Importance = 8;
            break;

        case EMatchEventType::GameEnded:
        {
            const bool bHomeWon = Event.HomeScore > Event.AwayScore;
            const FString& Winner = bHomeWon ? Snapshot.HomeTeam.Nickname : Snapshot.AwayTeam.Nickname;
            Line.Text = FString::Printf(TEXT("FINAL: %s win %d-%d%s!"),
                *Winner, FMath::Max(Event.HomeScore, Event.AwayScore),
                FMath::Min(Event.HomeScore, Event.AwayScore),
                Result.PeriodsPlayed > 4 ? TEXT(" in overtime") : TEXT(""));
            Line.Importance = 7;
            break;
        }
        default:
            continue;
        }

        Lines.Add(Line);
    }
    return Lines;
}

TArray<FCommentaryLine> FCommentaryService::GenerateHighlights(const FMatchResult& Result,
    const FMatchSnapshot& Snapshot, int32 MaxLines)
{
    TArray<FCommentaryLine> All = Generate(Result, Snapshot);
    All.Sort([](const FCommentaryLine& A, const FCommentaryLine& B) { return A.Importance > B.Importance; });
    if (All.Num() > MaxLines) { All.SetNum(MaxLines); }
    All.Sort([](const FCommentaryLine& A, const FCommentaryLine& B)
    {
        if (A.Period != B.Period) { return A.Period < B.Period; }
        return A.ClockSeconds > B.ClockSeconds;
    });
    return All;
}
