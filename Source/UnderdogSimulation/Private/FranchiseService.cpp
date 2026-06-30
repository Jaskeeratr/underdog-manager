#include "FranchiseService.h"

#include "RivalryService.h"

namespace
{
    FTeamState* FindTeam(FLeagueState& League, const FGuid& TeamId)
    {
        return League.Teams.FindByPredicate(
            [&TeamId](const FTeamState& Team) { return Team.TeamId == TeamId; });
    }

    const FTeamState* FindTeam(const FLeagueState& League, const FGuid& TeamId)
    {
        return League.Teams.FindByPredicate(
            [&TeamId](const FTeamState& Team) { return Team.TeamId == TeamId; });
    }

    int32 CalculateAttendance(const FLeagueState& League, const FTeamState& Home,
        const FTeamState& Away)
    {
        const int32 ArenaLevel = FFranchiseService::GetFacilityLevel(Home, EFacilityType::ArenaOperations);
        const int32 Capacity = 12000 + ArenaLevel * 1500;
        const FRivalry* Rivalry = FRivalryService::GetRivalry(League, Home.TeamId, Away.TeamId);
        const int32 RivalryBoost = Rivalry ? Rivalry->Intensity * 25 : 0;
        const int32 FormBoost = FMath::Clamp(Home.WinStreak, -5, 5) * 180;
        const int32 PricePenalty = static_cast<int32>(FMath::Max<int64>(0,
            Home.Franchise.Finances.TicketPriceMinorUnits - 4500) / 4);
        const int32 Demand = 3500 + Home.Franchise.Fanbase.Support * 95
            + Away.Franchise.Fanbase.Reputation * 22 + RivalryBoost + FormBoost - PricePenalty;
        return FMath::Clamp(Demand, 3000, Capacity);
    }

    void ApplyGameEconomics(FLeagueState& League, FTeamState& Home, FTeamState& Away,
        bool bHomeWon, bool bCloseGame)
    {
        const int32 Attendance = CalculateAttendance(League, Home, Away);
        const int32 ArenaLevel = FFranchiseService::GetFacilityLevel(Home, EFacilityType::ArenaOperations);
        const int64 TicketRevenue = Attendance * Home.Franchise.Finances.TicketPriceMinorUnits;
        const int64 ConcessionRevenue = Attendance * (900 + ArenaLevel * 125);
        const int64 HomeExpense = 30000000LL + Home.TotalSalary() / 22;
        const int64 AwayExpense = 12000000LL + Away.TotalSalary() / 22;

        Home.Franchise.Finances.LastAttendance = Attendance;
        Home.Franchise.Finances.LastGameRevenueMinorUnits = TicketRevenue + ConcessionRevenue;
        Home.Franchise.Finances.SeasonRevenueMinorUnits += TicketRevenue + ConcessionRevenue;
        Home.Franchise.Finances.SeasonExpensesMinorUnits += HomeExpense;
        Home.Franchise.Finances.CashMinorUnits += TicketRevenue + ConcessionRevenue - HomeExpense;
        Away.Franchise.Finances.LastAttendance = 0;
        Away.Franchise.Finances.LastGameRevenueMinorUnits = 0;
        Away.Franchise.Finances.SeasonExpensesMinorUnits += AwayExpense;
        Away.Franchise.Finances.CashMinorUnits -= AwayExpense;

        Home.OperatingBalanceMinorUnits = Home.Franchise.Finances.CashMinorUnits;
        Away.OperatingBalanceMinorUnits = Away.Franchise.Finances.CashMinorUnits;

        Home.Franchise.Fanbase.Support = FMath::Clamp(Home.Franchise.Fanbase.Support
            + (bHomeWon ? 2 : bCloseGame ? 0 : -1), 0, 100);
        Away.Franchise.Fanbase.Support = FMath::Clamp(Away.Franchise.Fanbase.Support
            + (!bHomeWon ? 1 : bCloseGame ? 0 : -1), 0, 100);
        Home.Franchise.Fanbase.Reputation = FMath::Clamp(Home.Franchise.Fanbase.Reputation
            + (bHomeWon ? 1 : 0), 0, 100);
        Away.Franchise.Fanbase.Reputation = FMath::Clamp(Away.Franchise.Fanbase.Reputation
            + (!bHomeWon ? 1 : 0), 0, 100);
    }
}

void FFranchiseService::InitializeTeam(FTeamState& Team, int32 MarketIndex)
{
    Team.Franchise = FFranchiseState();
    Team.Franchise.Finances.CashMinorUnits = Team.OperatingBalanceMinorUnits;
    Team.Franchise.Fanbase.Support = FMath::Clamp(38 + MarketIndex * 2, 35, 70);
    Team.Franchise.Fanbase.Reputation = FMath::Clamp(30 + MarketIndex * 3, 25, 75);
    Team.Franchise.Fanbase.SeasonTicketHolders = 2800 + MarketIndex * 220;

    for (int32 TypeIndex = 0; TypeIndex < 4; ++TypeIndex)
    {
        FFacilityState Facility;
        Facility.Type = static_cast<EFacilityType>(TypeIndex);
        Facility.Level = 1;
        Team.Franchise.Facilities.Add(Facility);
    }

    const EOwnerObjectiveType ObjectiveTypes[] = {
        EOwnerObjectiveType::WinGames, EOwnerObjectiveType::PositiveBalance,
        EOwnerObjectiveType::ImproveChemistry };
    for (EOwnerObjectiveType Type : ObjectiveTypes)
    {
        FOwnerObjective Objective;
        Objective.Type = Type;
        Objective.Target = Type == EOwnerObjectiveType::WinGames ? 8
            : Type == EOwnerObjectiveType::ImproveChemistry ? 60 : 1;
        Team.Franchise.Ownership.Objectives.Add(Objective);
    }
}

void FFranchiseService::ProcessCompletedGame(FLeagueState& League, const FMatchResult& Result)
{
    const FScheduledGame* Game = League.Schedule.FindByPredicate(
        [&Result](const FScheduledGame& Candidate) { return Candidate.GameId == Result.GameId; });
    if (!Game) { return; }
    FTeamState* Home = FindTeam(League, Game->HomeTeamId);
    FTeamState* Away = FindTeam(League, Game->AwayTeamId);
    if (!Home || !Away) { return; }

    const bool bHomeWon = Result.HomeScore > Result.AwayScore;
    ApplyGameEconomics(League, *Home, *Away, bHomeWon,
        FMath::Abs(Result.HomeScore - Result.AwayScore) <= 5);
    EvaluateOwnership(*Home, League.Phase);
    EvaluateOwnership(*Away, League.Phase);
}

void FFranchiseService::ResetForNewSeason(FLeagueState& League)
{
    for (FTeamState& Team : League.Teams)
    {
        // Gate receipts alone cannot support a professional club. Settle the
        // league media distribution and market-driven commercial income once
        // per season, while charging the staff payroll that was previously only
        // displayed in the UI. This keeps every recurring cost and revenue source
        // in the same integer minor-unit model.
        const int64 MediaDistribution = 4500000000LL;
        const int64 ReputationRevenue =
            static_cast<int64>(Team.Franchise.Fanbase.Reputation) * 15000000LL;
        const int64 SupportRevenue =
            static_cast<int64>(Team.Franchise.Fanbase.Support) * 8000000LL;
        const int64 CommercialRevenue = MediaDistribution + ReputationRevenue + SupportRevenue;
        const int64 StaffPayroll = FMath::Max<int64>(0, Team.Organization.AnnualPayrollMinorUnits);

        Team.Franchise.Finances.CashMinorUnits += CommercialRevenue - StaffPayroll;
        Team.Franchise.Finances.SeasonRevenueMinorUnits = CommercialRevenue;
        Team.Franchise.Finances.SeasonExpensesMinorUnits = StaffPayroll;
        Team.Franchise.Finances.LastGameRevenueMinorUnits = 0;
        Team.Franchise.Finances.LastAttendance = 0;
        Team.OperatingBalanceMinorUnits = Team.Franchise.Finances.CashMinorUnits;
        Team.Franchise.Ownership.Objectives.Reset();
        const EOwnerObjectiveType Types[] = { EOwnerObjectiveType::WinGames,
            EOwnerObjectiveType::PositiveBalance, EOwnerObjectiveType::ImproveChemistry };
        for (EOwnerObjectiveType Type : Types)
        {
            FOwnerObjective Objective;
            Objective.Type = Type;
            Objective.Target = Type == EOwnerObjectiveType::WinGames ? 8
                : Type == EOwnerObjectiveType::ImproveChemistry ? 60 : 1;
            Team.Franchise.Ownership.Objectives.Add(Objective);
        }
    }
}

bool FFranchiseService::SetTicketPrice(FLeagueState& League, const FGuid& TeamId,
    int64 TicketPriceMinorUnits, FString& OutError)
{
    FTeamState* Team = FindTeam(League, TeamId);
    if (!Team) { OutError = TEXT("Team not found."); return false; }
    if (TicketPriceMinorUnits < 2000 || TicketPriceMinorUnits > 15000)
    {
        OutError = TEXT("Ticket price must be between $20 and $150.");
        return false;
    }
    Team->Franchise.Finances.TicketPriceMinorUnits = TicketPriceMinorUnits;
    return true;
}

int32 FFranchiseService::GetFacilityLevel(const FTeamState& Team, EFacilityType Type)
{
    const FFacilityState* Facility = Team.Franchise.Facilities.FindByPredicate(
        [Type](const FFacilityState& Candidate) { return Candidate.Type == Type; });
    return Facility ? Facility->Level : 1;
}

int64 FFranchiseService::GetFacilityUpgradeCost(const FTeamState& Team, EFacilityType Type)
{
    const int32 Level = GetFacilityLevel(Team, Type);
    return Level >= 5 ? 0 : 175000000LL * Level * Level;
}

bool FFranchiseService::UpgradeFacility(FLeagueState& League, const FGuid& TeamId,
    EFacilityType Type, FString& OutError)
{
    FTeamState* Team = FindTeam(League, TeamId);
    if (!Team) { OutError = TEXT("Team not found."); return false; }
    FFacilityState* Facility = Team->Franchise.Facilities.FindByPredicate(
        [Type](const FFacilityState& Candidate) { return Candidate.Type == Type; });
    if (!Facility) { OutError = TEXT("Facility is not configured."); return false; }
    if (Facility->Level >= 5) { OutError = TEXT("Facility is already at maximum level."); return false; }
    const int64 Cost = GetFacilityUpgradeCost(*Team, Type);
    if (Team->Franchise.Finances.CashMinorUnits < Cost)
    {
        OutError = TEXT("Insufficient operating cash for this upgrade.");
        return false;
    }
    Team->Franchise.Finances.CashMinorUnits -= Cost;
    Team->Franchise.Finances.SeasonExpensesMinorUnits += Cost;
    Team->OperatingBalanceMinorUnits = Team->Franchise.Finances.CashMinorUnits;
    Facility->Level++;
    return true;
}

int64 FFranchiseService::ProjectNextHomeGameRevenue(const FLeagueState& League,
    const FGuid& TeamId, const FGuid& OpponentId)
{
    const FTeamState* Home = FindTeam(League, TeamId);
    const FTeamState* Away = FindTeam(League, OpponentId);
    if (!Home || !Away) { return 0; }
    const int32 Attendance = CalculateAttendance(League, *Home, *Away);
    const int32 ArenaLevel = GetFacilityLevel(*Home, EFacilityType::ArenaOperations);
    return Attendance * (Home->Franchise.Finances.TicketPriceMinorUnits + 900 + ArenaLevel * 125);
}

void FFranchiseService::EvaluateOwnership(FTeamState& Team, ESeasonPhase Phase)
{
    int32 Completed = 0;
    for (FOwnerObjective& Objective : Team.Franchise.Ownership.Objectives)
    {
        switch (Objective.Type)
        {
        case EOwnerObjectiveType::WinGames: Objective.Current = Team.Wins; break;
        case EOwnerObjectiveType::ReachPlayoffs:
            Objective.Current = Phase != ESeasonPhase::RegularSeason ? 1 : 0; break;
        case EOwnerObjectiveType::PositiveBalance:
            Objective.Current = Team.Franchise.Finances.OperatingProfit() >= 0 ? 1 : 0; break;
        case EOwnerObjectiveType::ImproveChemistry: Objective.Current = Team.Chemistry; break;
        }
        Objective.bCompleted = Objective.Current >= Objective.Target;
        Completed += Objective.bCompleted ? 1 : 0;
    }
    const int32 Games = Team.Wins + Team.Losses;
    const int32 RecordScore = Games > 0 ? Team.Wins * 30 / Games : 15;
    Team.Franchise.Ownership.Confidence = FMath::Clamp(35 + RecordScore + Completed * 10, 0, 100);
}
