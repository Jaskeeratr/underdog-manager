#include "HighlightDirectorService.h"

FString FHighlightDirectorService::FindPlayerName(const FMatchSnapshot& Snapshot, const FGuid& PlayerId)
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

EHighlightTemplate FHighlightDirectorService::ClassifyEvent(const FMatchEvent& Event, const FMatchResult& Result)
{
    const bool bClutch = Event.ClockSeconds < 120 && FMath::Abs(Event.HomeScore - Event.AwayScore) <= 5;
    const bool bFinalPossession = Event.ClockSeconds < 30 &&
        (Event.Period >= 4 || Event.Period >= Result.PeriodsPlayed);

    switch (Event.Type)
    {
    case EMatchEventType::ThreePointMade:
        if (bFinalPossession) return EHighlightTemplate::FinalPossession;
        if (bClutch) return EHighlightTemplate::ClutchBasket;
        return EHighlightTemplate::ThreePointer;

    case EMatchEventType::TwoPointMade:
        if (bFinalPossession) return EHighlightTemplate::FinalPossession;
        if (bClutch) return EHighlightTemplate::ClutchBasket;
        if (Event.SecondaryPlayerId.IsValid()) return EHighlightTemplate::AssistedBasket;
        return EHighlightTemplate::DriveAndFinish;

    case EMatchEventType::Block:
        return EHighlightTemplate::BlockPlay;

    case EMatchEventType::Steal:
        return EHighlightTemplate::StealFastBreak;

    case EMatchEventType::FreeThrowMade:
        if (bClutch) return EHighlightTemplate::FreeThrows;
        return EHighlightTemplate::GenericFallback;

    default:
        return EHighlightTemplate::GenericFallback;
    }
}

ECameraPreset FHighlightDirectorService::SelectCamera(EHighlightTemplate Template)
{
    switch (Template)
    {
    case EHighlightTemplate::ThreePointer: return ECameraPreset::Wide;
    case EHighlightTemplate::DriveAndFinish: return ECameraPreset::FollowBall;
    case EHighlightTemplate::AssistedBasket: return ECameraPreset::Broadcast;
    case EHighlightTemplate::BlockPlay: return ECameraPreset::Baseline;
    case EHighlightTemplate::StealFastBreak: return ECameraPreset::Wide;
    case EHighlightTemplate::FreeThrows: return ECameraPreset::CloseUp;
    case EHighlightTemplate::ClutchBasket: return ECameraPreset::CloseUp;
    case EHighlightTemplate::FinalPossession: return ECameraPreset::HighAngle;
    default: return ECameraPreset::Broadcast;
    }
}

float FHighlightDirectorService::EstimateDuration(EHighlightTemplate Template)
{
    switch (Template)
    {
    case EHighlightTemplate::ThreePointer: return 4.0f;
    case EHighlightTemplate::DriveAndFinish: return 4.5f;
    case EHighlightTemplate::AssistedBasket: return 5.0f;
    case EHighlightTemplate::BlockPlay: return 3.5f;
    case EHighlightTemplate::StealFastBreak: return 5.5f;
    case EHighlightTemplate::FreeThrows: return 3.0f;
    case EHighlightTemplate::ClutchBasket: return 5.0f;
    case EHighlightTemplate::FinalPossession: return 6.0f;
    default: return 3.0f;
    }
}

int32 FHighlightDirectorService::ScoreImportance(const FMatchEvent& Event, const FMatchResult& Result)
{
    int32 Score = 0;

    const int32 ScoreDiff = FMath::Abs(Event.HomeScore - Event.AwayScore);
    if (ScoreDiff <= 3) Score += 30;
    else if (ScoreDiff <= 8) Score += 15;

    if (Event.ClockSeconds < 60) Score += 25;
    else if (Event.ClockSeconds < 180) Score += 10;

    if (Event.Period > 4) Score += 20;

    switch (Event.Type)
    {
    case EMatchEventType::ThreePointMade: Score += 20; break;
    case EMatchEventType::Block: Score += 25; break;
    case EMatchEventType::Steal: Score += 15; break;
    case EMatchEventType::TwoPointMade: Score += 10; break;
    case EMatchEventType::FreeThrowMade: Score += 5; break;
    default: break;
    }

    const bool bLeadChange = (Event.HomeScore > Event.AwayScore) !=
        ((Event.HomeScore - (Event.Type == EMatchEventType::ThreePointMade ? 3 :
            Event.Type == EMatchEventType::TwoPointMade ? 2 : 1)) > Event.AwayScore);
    if (bLeadChange) Score += 20;

    return Score;
}

TArray<FHighlightCue> FHighlightDirectorService::BuildHighlights(
    const FMatchResult& Result, const FMatchSnapshot& Snapshot, int32 MaxCues)
{
    struct FScoredEvent
    {
        int32 EventIndex;
        int32 Importance;
        EHighlightTemplate Template;
    };

    TArray<FScoredEvent> Candidates;
    for (int32 I = 0; I < Result.Events.Num(); ++I)
    {
        const FMatchEvent& Event = Result.Events[I];
        EHighlightTemplate Template = ClassifyEvent(Event, Result);
        if (Template == EHighlightTemplate::GenericFallback &&
            Event.Type != EMatchEventType::Block && Event.Type != EMatchEventType::Steal)
        {
            continue;
        }

        FScoredEvent Scored;
        Scored.EventIndex = I;
        Scored.Importance = ScoreImportance(Event, Result);
        Scored.Template = Template;
        Candidates.Add(Scored);
    }

    Candidates.Sort([](const FScoredEvent& A, const FScoredEvent& B)
    {
        return A.Importance > B.Importance;
    });

    if (Candidates.Num() > MaxCues)
    {
        Candidates.SetNum(MaxCues);
    }

    Candidates.Sort([&Result](const FScoredEvent& A, const FScoredEvent& B)
    {
        return Result.Events[A.EventIndex].Sequence < Result.Events[B.EventIndex].Sequence;
    });

    if (Candidates.Num() == 0)
    {
        FHighlightCue Fallback;
        Fallback.Template = EHighlightTemplate::GenericFallback;
        Fallback.Camera = ECameraPreset::Broadcast;
        Fallback.Period = Result.PeriodsPlayed;
        Fallback.ClockSeconds = 0;
        Fallback.HomeScoreBefore = Result.HomeScore;
        Fallback.AwayScoreBefore = Result.AwayScore;
        Fallback.HomeScoreAfter = Result.HomeScore;
        Fallback.AwayScoreAfter = Result.AwayScore;
        Fallback.Description = TEXT("Game concluded");
        Fallback.Importance = 0;
        Fallback.PlaybackDuration = 3.0f;
        return { Fallback };
    }

    TArray<FHighlightCue> Cues;
    for (const FScoredEvent& Scored : Candidates)
    {
        const FMatchEvent& Event = Result.Events[Scored.EventIndex];
        FHighlightCue Cue;
        Cue.Template = Scored.Template;
        Cue.Camera = SelectCamera(Scored.Template);
        Cue.Period = Event.Period;
        Cue.ClockSeconds = Event.ClockSeconds;
        Cue.PossessionTeamId = Event.TeamId;
        Cue.PrimaryPlayerId = Event.PrimaryPlayerId;
        Cue.SecondaryPlayerId = Event.SecondaryPlayerId;
        Cue.PrimaryPlayerName = FindPlayerName(Snapshot, Event.PrimaryPlayerId);
        Cue.SecondaryPlayerName = Event.SecondaryPlayerId.IsValid()
            ? FindPlayerName(Snapshot, Event.SecondaryPlayerId) : FString();
        Cue.Importance = Scored.Importance;
        Cue.PlaybackDuration = EstimateDuration(Scored.Template);
        Cue.bOutcome = (Event.Type == EMatchEventType::TwoPointMade ||
            Event.Type == EMatchEventType::ThreePointMade ||
            Event.Type == EMatchEventType::FreeThrowMade ||
            Event.Type == EMatchEventType::Block ||
            Event.Type == EMatchEventType::Steal);

        int32 PrevHomeScore = Event.HomeScore;
        int32 PrevAwayScore = Event.AwayScore;
        if (Scored.EventIndex > 0)
        {
            PrevHomeScore = Result.Events[Scored.EventIndex - 1].HomeScore;
            PrevAwayScore = Result.Events[Scored.EventIndex - 1].AwayScore;
        }
        Cue.HomeScoreBefore = PrevHomeScore;
        Cue.AwayScoreBefore = PrevAwayScore;
        Cue.HomeScoreAfter = Event.HomeScore;
        Cue.AwayScoreAfter = Event.AwayScore;

        const FString TeamName = Event.TeamId == Snapshot.HomeTeam.TeamId
            ? Snapshot.HomeTeam.Nickname : Snapshot.AwayTeam.Nickname;

        switch (Scored.Template)
        {
        case EHighlightTemplate::ThreePointer:
            Cue.Description = FString::Printf(TEXT("%s drains a three for %s!"), *Cue.PrimaryPlayerName, *TeamName);
            break;
        case EHighlightTemplate::DriveAndFinish:
            Cue.Description = FString::Printf(TEXT("%s drives and scores!"), *Cue.PrimaryPlayerName);
            break;
        case EHighlightTemplate::AssistedBasket:
            Cue.Description = FString::Printf(TEXT("%s finds %s for the bucket!"),
                *Cue.SecondaryPlayerName, *Cue.PrimaryPlayerName);
            break;
        case EHighlightTemplate::BlockPlay:
            Cue.Description = FString::Printf(TEXT("%s with a huge block on %s!"),
                *Cue.PrimaryPlayerName, *Cue.SecondaryPlayerName);
            break;
        case EHighlightTemplate::StealFastBreak:
            Cue.Description = FString::Printf(TEXT("%s steals it and pushes the break!"), *Cue.PrimaryPlayerName);
            break;
        case EHighlightTemplate::FreeThrows:
            Cue.Description = FString::Printf(TEXT("%s steps to the line in a crucial moment"), *Cue.PrimaryPlayerName);
            break;
        case EHighlightTemplate::ClutchBasket:
            Cue.Description = FString::Printf(TEXT("CLUTCH! %s delivers when it matters!"), *Cue.PrimaryPlayerName);
            break;
        case EHighlightTemplate::FinalPossession:
            Cue.Description = FString::Printf(TEXT("Final possession — %s with the shot!"), *Cue.PrimaryPlayerName);
            break;
        default:
            Cue.Description = FString::Printf(TEXT("%s makes a play"), *Cue.PrimaryPlayerName);
            break;
        }

        Cues.Add(Cue);
    }

    return Cues;
}

bool FHighlightDirectorService::ValidateCues(
    const TArray<FHighlightCue>& Cues, const FMatchResult& Result, FString& OutError)
{
    for (int32 I = 0; I < Cues.Num(); ++I)
    {
        const FHighlightCue& Cue = Cues[I];

        if (Cue.PrimaryPlayerName.IsEmpty() || Cue.PrimaryPlayerName == TEXT("Unknown"))
        {
            OutError = FString::Printf(TEXT("Cue %d has invalid primary player reference"), I);
            return false;
        }

        if (I > 0)
        {
            const FHighlightCue& Prev = Cues[I - 1];
            if (Cue.Period < Prev.Period ||
                (Cue.Period == Prev.Period && Cue.ClockSeconds > Prev.ClockSeconds))
            {
                OutError = FString::Printf(TEXT("Cue %d is not chronological (period %d %ds after period %d %ds)"),
                    I, Cue.Period, Cue.ClockSeconds, Prev.Period, Prev.ClockSeconds);
                return false;
            }
        }
    }

    if (Cues.Num() > 0)
    {
        const FHighlightCue& Last = Cues.Last();
        if (Last.HomeScoreAfter > Result.HomeScore || Last.AwayScoreAfter > Result.AwayScore)
        {
            OutError = TEXT("Final cue scores exceed match result");
            return false;
        }
    }

    return true;
}
