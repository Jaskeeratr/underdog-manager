#include "ContractService.h"

TArray<FExtensionOffer> FContractService::GetEligibleExtensions(const FLeagueState& League, const FGuid& TeamId)
{
    TArray<FExtensionOffer> Results;
    const FTeamState* Team = League.Teams.FindByPredicate(
        [&TeamId](const FTeamState& T) { return T.TeamId == TeamId; });
    if (!Team) { return Results; }

    for (const FPlayerProfile& Player : Team->Players)
    {
        if (Player.Contract.YearsRemaining <= 1)
        {
            const FAthleteState* State = Team->PlayerStates.FindByPredicate(
                [&Player](const FAthleteState& S) { return S.PlayerId == Player.PlayerId; });
            Results.Add(CalculateAskingPrice(Player, State));
        }
    }
    return Results;
}

bool FContractService::OfferExtension(FLeagueState& League, const FGuid& TeamId,
    const FGuid& PlayerId, int64 OfferedSalary, int32 OfferedYears, FString& OutError)
{
    FTeamState* Team = League.Teams.FindByPredicate(
        [&TeamId](const FTeamState& T) { return T.TeamId == TeamId; });
    if (!Team) { OutError = TEXT("Team not found."); return false; }

    FPlayerProfile* Player = Team->Players.FindByPredicate(
        [&PlayerId](const FPlayerProfile& P) { return P.PlayerId == PlayerId; });
    if (!Player) { OutError = TEXT("Player not found on this team."); return false; }

    if (Player->Contract.YearsRemaining > 1)
    {
        OutError = TEXT("Player is not eligible for extension (more than 1 year remaining).");
        return false;
    }

    const FAthleteState* State = Team->PlayerStates.FindByPredicate(
        [&PlayerId](const FAthleteState& S) { return S.PlayerId == PlayerId; });
    const FExtensionOffer Asking = CalculateAskingPrice(*Player, State);

    if (OfferedYears < 1 || OfferedYears > 5)
    {
        OutError = TEXT("Contract length must be 1-5 years.");
        return false;
    }

    const int64 NewTotalSalary = Team->TotalSalary() - Player->Contract.SalaryMinorUnits + OfferedSalary;
    if (NewTotalSalary > Team->LuxuryTaxThreshold())
    {
        OutError = TEXT("This extension would exceed the luxury tax threshold.");
        return false;
    }

    const float SalaryRatio = static_cast<float>(OfferedSalary) / FMath::Max(1LL, Asking.AskingSalary);
    const float YearsRatio = static_cast<float>(OfferedYears) / FMath::Max(1, Asking.AskingYears);

    const float Satisfaction = SalaryRatio * 0.7f + YearsRatio * 0.3f;
    const int32 MoraleBonus = State ? FMath::Max(0, (State->Morale - 50) / 10) : 0;

    if (Satisfaction >= 0.85f || (Satisfaction >= 0.75f && MoraleBonus >= 2))
    {
        Player->Contract.SalaryMinorUnits = OfferedSalary;
        Player->Contract.YearsRemaining = OfferedYears;
        return true;
    }

    OutError = FString::Printf(TEXT("%s declined the offer. They want closer to $%lldM/%dy."),
        *Player->DisplayName, Asking.AskingSalary / 1000000, Asking.AskingYears);
    return false;
}

FExtensionOffer FContractService::CalculateAskingPrice(const FPlayerProfile& Player, const FAthleteState* State)
{
    FExtensionOffer Offer;
    Offer.PlayerId = Player.PlayerId;
    Offer.PlayerName = Player.DisplayName;

    const int32 Overall = Player.Ratings.Overall();
    const int32 MoraleMod = State ? (State->Morale - 50) / 5 : 0;

    int64 BaseSalary = static_cast<int64>(Overall) * 2500000LL;
    if (Overall >= 75) { BaseSalary = static_cast<int64>(Overall) * 3500000LL; }
    if (Overall >= 85) { BaseSalary = static_cast<int64>(Overall) * 4500000LL; }

    if (Player.Age >= 33) { BaseSalary = BaseSalary * 80 / 100; }
    if (Player.Age <= 24 && Player.Ratings.Potential >= 70) { BaseSalary = BaseSalary * 115 / 100; }

    Offer.AskingSalary = BaseSalary;
    Offer.AskingYears = Overall >= 75 ? 4 : Overall >= 60 ? 3 : 2;
    if (Player.Age >= 33) { Offer.AskingYears = FMath::Min(Offer.AskingYears, 2); }

    if (MoraleMod > 0) { Offer.AskingSalary = Offer.AskingSalary * (100 - MoraleMod * 2) / 100; }

    return Offer;
}
