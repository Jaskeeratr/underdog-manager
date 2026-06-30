#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API FStaffService
{
public:
    static void InitializeLeague(FLeagueState& League);
    static void ProcessRound(FLeagueState& League, const FGuid& PlayerTeamId);
    static void ProcessOffseason(FLeagueState& League);
    static bool HireStaff(FLeagueState& League, const FGuid& TeamId,
        const FGuid& StaffId, int64 OfferedSalary, int32 OfferedYears, FString& OutError);
    static bool FireStaff(FLeagueState& League, const FGuid& TeamId,
        EStaffRole Role, FString& OutError);
    static const FStaffMember* FindStaff(const FTeamState& Team, EStaffRole Role);
    static int32 GetDevelopmentBonus(const FTeamState& Team);
    static int32 GetMedicalBonus(const FTeamState& Team);
    static int32 GetScoutingBonus(const FTeamState& Team);
    static int32 GetTacticalBonus(const FTeamState& Team, bool bOffense);
    static void RecalculateOrganization(FTeamState& Team);
};
