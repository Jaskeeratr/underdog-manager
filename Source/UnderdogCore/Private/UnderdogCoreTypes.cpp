#include "UnderdogCoreTypes.h"

void FPlayerRatings::Clamp()
{
    int32* Values[] = { &InsideScoring, &OutsideShooting, &Playmaking, &PerimeterDefense,
        &InteriorDefense, &Rebounding, &Athleticism, &Stamina, &Potential, &WorkEthic };
    for (int32* Value : Values) { *Value = FMath::Clamp(*Value, 1, 99); }
}

int32 FPlayerRatings::Overall() const
{
    const int32 Total = InsideScoring + OutsideShooting + Playmaking + PerimeterDefense
        + InteriorDefense + Rebounding + Athleticism + Stamina;
    return FMath::RoundToInt(static_cast<double>(Total) / 8.0);
}

int32 FStaffRatings::OverallForRole(EStaffRole Role) const
{
    switch (Role)
    {
    case EStaffRole::HeadCoach: return (Offense + Defense + Motivation + TacticalFlexibility) / 4;
    case EStaffRole::OffensiveCoach: return (Offense * 2 + Development + TacticalFlexibility) / 4;
    case EStaffRole::DefensiveCoach: return (Defense * 2 + Development + TacticalFlexibility) / 4;
    case EStaffRole::DevelopmentCoach: return (Development * 2 + Motivation + TacticalFlexibility) / 4;
    case EStaffRole::HeadScout: return (Scouting * 3 + TacticalFlexibility) / 4;
    case EStaffRole::MedicalDirector: return (Medical * 3 + Motivation) / 4;
    default: return 50;
    }
}

bool FRotationPlan::IsValid(FString& OutError) const
{
    if (OrderedPlayers.Num() < 5 || OrderedPlayers.Num() > 15)
    {
        OutError = TEXT("A rotation must contain between 5 and 15 players.");
        return false;
    }
    TSet<FGuid> Unique;
    int32 TotalMinutes = 0;
    for (const FGuid& Id : OrderedPlayers)
    {
        if (!Id.IsValid() || Unique.Contains(Id))
        {
            OutError = TEXT("Rotation player IDs must be valid and unique.");
            return false;
        }
        Unique.Add(Id);
        const int32* Minutes = TargetMinutes.Find(Id);
        if (!Minutes || *Minutes < 0 || *Minutes > 48)
        {
            OutError = TEXT("Every rotation player requires a target from 0 to 48 minutes.");
            return false;
        }
        TotalMinutes += *Minutes;
    }
    if (TotalMinutes != 240)
    {
        OutError = FString::Printf(TEXT("Rotation minutes total %d; expected 240."), TotalMinutes);
        return false;
    }
    return true;
}

bool FMatchResult::Validate(FString& OutError) const
{
    if (!GameId.IsValid() || HomeScore < 0 || AwayScore < 0 || HomeScore == AwayScore)
    {
        OutError = TEXT("Match identity or final score is invalid.");
        return false;
    }
    auto ValidateSide = [&OutError](const TArray<FPlayerBoxScore>& Scores, int32 Expected)
    {
        int32 Total = 0;
        for (const FPlayerBoxScore& Box : Scores)
        {
            const int32 Calculated = (Box.FieldGoalsMade - Box.ThreePointersMade) * 2
                + Box.ThreePointersMade * 3 + Box.FreeThrowsMade;
            if (Calculated != Box.Points || Box.FieldGoalsMade > Box.FieldGoalsAttempted
                || Box.ThreePointersMade > Box.FieldGoalsMade || Box.FreeThrowsMade > Box.FreeThrowsAttempted
                || Box.OffensiveRebounds > Box.Rebounds || Box.Points < 0 || Box.Rebounds < 0
                || Box.Assists < 0 || Box.Steals < 0 || Box.Blocks < 0 || Box.Turnovers < 0 || Box.Fouls < 0)
            {
                OutError = TEXT("A player box score is internally inconsistent.");
                return false;
            }
            Total += Box.Points;
        }
        if (Total != Expected)
        {
            OutError = TEXT("Player points do not reconcile with the team score.");
            return false;
        }
        return true;
    };
    if (!ValidateSide(HomeBoxScore, HomeScore) || !ValidateSide(AwayBoxScore, AwayScore)) { return false; }
    if (Events.Num() < 2 || Events[0].Type != EMatchEventType::GameStarted
        || Events.Last().Type != EMatchEventType::GameEnded)
    {
        OutError = TEXT("Match event log is incomplete.");
        return false;
    }
    for (int32 Index = 0; Index < Events.Num(); ++Index)
    {
        if (Events[Index].Sequence != Index || Events[Index].HomeScore < 0 || Events[Index].AwayScore < 0)
        {
            OutError = TEXT("Match event sequence or running score is invalid.");
            return false;
        }
    }
    if (Events.Last().HomeScore != HomeScore || Events.Last().AwayScore != AwayScore)
    {
        OutError = TEXT("Final event score does not match the result.");
        return false;
    }
    return true;
}
