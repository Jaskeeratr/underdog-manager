#include "StaffService.h"

#include "DeterministicRandom.h"

namespace
{
    const TCHAR* FirstNames[] = { TEXT("Avery"), TEXT("Jordan"), TEXT("Morgan"), TEXT("Casey"),
        TEXT("Cameron"), TEXT("Reese"), TEXT("Drew"), TEXT("Taylor"), TEXT("Quinn"), TEXT("Skyler") };
    const TCHAR* LastNames[] = { TEXT("Bishop"), TEXT("Mercer"), TEXT("Holloway"), TEXT("Santos"),
        TEXT("Okafor"), TEXT("Navarro"), TEXT("Whitaker"), TEXT("Park"), TEXT("Ellison"), TEXT("Monroe") };

    FGuid MakeGuid(FDeterministicRandom& Random)
    {
        return FGuid(Random.NextUInt32(), Random.NextUInt32(), Random.NextUInt32(), Random.NextUInt32());
    }

    FStaffMember GenerateStaff(FDeterministicRandom& Random, EStaffRole Role, int32 Quality)
    {
        FStaffMember Staff;
        Staff.StaffId = MakeGuid(Random);
        Staff.DisplayName = FString::Printf(TEXT("%s %s"), FirstNames[Random.Range(0, 9)], LastNames[Random.Range(0, 9)]);
        Staff.Age = Random.Range(34, 65);
        Staff.Role = Role;
        Staff.Personality = static_cast<EStaffPersonality>(Random.Range(0, 4));
        Staff.Ratings.Offense = FMath::Clamp(Quality + Random.Range(-12, 12), 25, 95);
        Staff.Ratings.Defense = FMath::Clamp(Quality + Random.Range(-12, 12), 25, 95);
        Staff.Ratings.Development = FMath::Clamp(Quality + Random.Range(-12, 12), 25, 95);
        Staff.Ratings.Motivation = FMath::Clamp(Quality + Random.Range(-12, 12), 25, 95);
        Staff.Ratings.Scouting = FMath::Clamp(Quality + Random.Range(-12, 12), 25, 95);
        Staff.Ratings.Medical = FMath::Clamp(Quality + Random.Range(-12, 12), 25, 95);
        Staff.Ratings.TacticalFlexibility = FMath::Clamp(Quality + Random.Range(-12, 12), 25, 95);
        Staff.Reputation = Staff.Ratings.OverallForRole(Role);
        Staff.Contract.SalaryMinorUnits = static_cast<int64>(Staff.Reputation) * 1400000LL;
        Staff.Contract.YearsRemaining = Random.Range(1, 3);
        return Staff;
    }

    FTeamState* FindTeam(FLeagueState& League, const FGuid& TeamId)
    {
        return League.Teams.FindByPredicate(
            [&TeamId](const FTeamState& Candidate) { return Candidate.TeamId == TeamId; });
    }
}

void FStaffService::InitializeLeague(FLeagueState& League)
{
    FDeterministicRandom Random(static_cast<uint64>(League.LeagueSeed) ^ 0x5354414646ULL);
    for (int32 TeamIndex = 0; TeamIndex < League.Teams.Num(); ++TeamIndex)
    {
        FTeamState& Team = League.Teams[TeamIndex];
        Team.Organization = FOrganizationState();
        for (int32 RoleIndex = 0; RoleIndex < 6; ++RoleIndex)
        {
            Team.Organization.Staff.Add(GenerateStaff(Random,
                static_cast<EStaffRole>(RoleIndex), 45 + TeamIndex * 2));
        }
        RecalculateOrganization(Team);
    }

    League.StaffMarket.Reset();
    for (int32 CandidateIndex = 0; CandidateIndex < 18; ++CandidateIndex)
    {
        League.StaffMarket.Add(GenerateStaff(Random,
            static_cast<EStaffRole>(CandidateIndex % 6), Random.Range(45, 82)));
    }
}

const FStaffMember* FStaffService::FindStaff(const FTeamState& Team, EStaffRole Role)
{
    return Team.Organization.Staff.FindByPredicate(
        [Role](const FStaffMember& Staff) { return Staff.Role == Role; });
}

void FStaffService::RecalculateOrganization(FTeamState& Team)
{
    Team.Organization.AnnualPayrollMinorUnits = 0;
    int32 Motivation = 0;
    for (const FStaffMember& Staff : Team.Organization.Staff)
    {
        Team.Organization.AnnualPayrollMinorUnits += Staff.Contract.SalaryMinorUnits;
        Motivation += Staff.Ratings.Motivation;
    }
    Team.Organization.StaffChemistry = Team.Organization.Staff.IsEmpty()
        ? 25 : FMath::Clamp(Motivation / Team.Organization.Staff.Num(), 0, 100);
}

int32 FStaffService::GetDevelopmentBonus(const FTeamState& Team)
{
    const FStaffMember* Staff = FindStaff(Team, EStaffRole::DevelopmentCoach);
    return Staff ? FMath::Clamp((Staff->Ratings.Development - 50) * 8, -200, 350) : -200;
}

int32 FStaffService::GetMedicalBonus(const FTeamState& Team)
{
    const FStaffMember* Staff = FindStaff(Team, EStaffRole::MedicalDirector);
    return Staff ? FMath::Clamp((Staff->Ratings.Medical - 50) * 5, -125, 225) : -125;
}

int32 FStaffService::GetScoutingBonus(const FTeamState& Team)
{
    const FStaffMember* Staff = FindStaff(Team, EStaffRole::HeadScout);
    return Staff ? FMath::Clamp((Staff->Ratings.Scouting - 50) / 15, -2, 3) : -2;
}

int32 FStaffService::GetTacticalBonus(const FTeamState& Team, bool bOffense)
{
    const FStaffMember* Head = FindStaff(Team, EStaffRole::HeadCoach);
    const FStaffMember* Specialist = FindStaff(Team,
        bOffense ? EStaffRole::OffensiveCoach : EStaffRole::DefensiveCoach);
    const int32 Rating = ((Head ? (bOffense ? Head->Ratings.Offense : Head->Ratings.Defense) : 35)
        + (Specialist ? (bOffense ? Specialist->Ratings.Offense : Specialist->Ratings.Defense) : 35)) / 2;
    return FMath::Clamp((Rating - 50) / 5, -3, 6);
}

bool FStaffService::HireStaff(FLeagueState& League, const FGuid& TeamId,
    const FGuid& StaffId, int64 OfferedSalary, int32 OfferedYears, FString& OutError)
{
    FTeamState* Team = FindTeam(League, TeamId);
    const int32 CandidateIndex = League.StaffMarket.IndexOfByPredicate(
        [&StaffId](const FStaffMember& Staff) { return Staff.StaffId == StaffId; });
    if (!Team || CandidateIndex == INDEX_NONE) { OutError = TEXT("Staff candidate not found."); return false; }
    if (OfferedYears < 1 || OfferedYears > 3) { OutError = TEXT("Staff contracts must be one to three years."); return false; }

    FStaffMember Candidate = League.StaffMarket[CandidateIndex];
    if (OfferedSalary < Candidate.Contract.SalaryMinorUnits)
    {
        OutError = TEXT("The candidate rejected the offer because the salary was below their expectation.");
        return false;
    }
    const int64 ExistingSalary = FindStaff(*Team, Candidate.Role)
        ? FindStaff(*Team, Candidate.Role)->Contract.SalaryMinorUnits : 0;
    const int64 FirstYearCost = OfferedSalary + ExistingSalary / 2;
    if (Team->Franchise.Finances.CashMinorUnits < FirstYearCost)
    {
        OutError = TEXT("The organization cannot afford this hire and staff termination cost.");
        return false;
    }

    Team->Organization.Staff.RemoveAll(
        [&Candidate](const FStaffMember& Staff) { return Staff.Role == Candidate.Role; });
    Candidate.Contract.SalaryMinorUnits = OfferedSalary;
    Candidate.Contract.YearsRemaining = OfferedYears;
    Team->Organization.Staff.Add(Candidate);
    Team->Franchise.Finances.CashMinorUnits -= FirstYearCost;
    Team->OperatingBalanceMinorUnits = Team->Franchise.Finances.CashMinorUnits;
    League.StaffMarket.RemoveAt(CandidateIndex);
    RecalculateOrganization(*Team);
    return true;
}

bool FStaffService::FireStaff(FLeagueState& League, const FGuid& TeamId,
    EStaffRole Role, FString& OutError)
{
    FTeamState* Team = FindTeam(League, TeamId);
    if (!Team) { OutError = TEXT("Team not found."); return false; }
    const int32 Index = Team->Organization.Staff.IndexOfByPredicate(
        [Role](const FStaffMember& Staff) { return Staff.Role == Role; });
    if (Index == INDEX_NONE) { OutError = TEXT("That staff role is already vacant."); return false; }
    const int64 Buyout = Team->Organization.Staff[Index].Contract.SalaryMinorUnits / 2;
    if (Team->Franchise.Finances.CashMinorUnits < Buyout) { OutError = TEXT("Insufficient cash for the buyout."); return false; }
    FStaffMember Released = Team->Organization.Staff[Index];
    Released.Contract.YearsRemaining = 1;
    Team->Organization.Staff.RemoveAt(Index);
    League.StaffMarket.Add(Released);
    Team->Franchise.Finances.CashMinorUnits -= Buyout;
    Team->OperatingBalanceMinorUnits = Team->Franchise.Finances.CashMinorUnits;
    RecalculateOrganization(*Team);
    return true;
}

void FStaffService::ProcessRound(FLeagueState& League, const FGuid& PlayerTeamId)
{
    for (FTeamState& Team : League.Teams)
    {
        const FStaffMember* Head = FindStaff(Team, EStaffRole::HeadCoach);
        const int32 Flexibility = Head ? Head->Ratings.TacticalFlexibility : 30;
        Team.Organization.TacticalFamiliarity = FMath::Clamp(
            Team.Organization.TacticalFamiliarity + 1 + Flexibility / 40, 0, 100);
        if (Team.TeamId == PlayerTeamId) { continue; }
        for (int32 RoleIndex = 0; RoleIndex < 6; ++RoleIndex)
        {
            const EStaffRole Role = static_cast<EStaffRole>(RoleIndex);
            if (FindStaff(Team, Role)) { continue; }
            int32 BestIndex = INDEX_NONE;
            int32 BestValue = -1;
            for (int32 Index = 0; Index < League.StaffMarket.Num(); ++Index)
            {
                if (League.StaffMarket[Index].Role != Role) { continue; }
                const int32 Value = League.StaffMarket[Index].Ratings.OverallForRole(Role);
                if (Value > BestValue) { BestValue = Value; BestIndex = Index; }
            }
            if (BestIndex != INDEX_NONE)
            {
                FString Error;
                HireStaff(League, Team.TeamId, League.StaffMarket[BestIndex].StaffId,
                    League.StaffMarket[BestIndex].Contract.SalaryMinorUnits, 2, Error);
            }
        }
    }
}

void FStaffService::ProcessOffseason(FLeagueState& League)
{
    for (FTeamState& Team : League.Teams)
    {
        for (int32 Index = Team.Organization.Staff.Num() - 1; Index >= 0; --Index)
        {
            FStaffMember& Staff = Team.Organization.Staff[Index];
            Staff.Contract.YearsRemaining--;
            if (Staff.Contract.YearsRemaining <= 0 || Staff.Age >= 68)
            {
                if (Staff.Age < 68) { League.StaffMarket.Add(Staff); }
                Team.Organization.Staff.RemoveAt(Index);
            }
            else { Staff.Age++; }
        }
        RecalculateOrganization(Team);
    }
}
