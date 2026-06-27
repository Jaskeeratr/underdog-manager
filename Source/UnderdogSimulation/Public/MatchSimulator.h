#pragma once

#include "CoreMinimal.h"
#include "UnderdogCoreTypes.h"

class UNDERDOGSIMULATION_API IMatchSimulator
{
public:
    virtual ~IMatchSimulator() = default;
    virtual FMatchResult Simulate(const FMatchSnapshot& Snapshot) const = 0;
};

class UNDERDOGSIMULATION_API FPossessionMatchSimulator final : public IMatchSimulator
{
public:
    virtual FMatchResult Simulate(const FMatchSnapshot& Snapshot) const override;

private:
    static const FPlayerProfile& SelectShooter(const FTeamState& Team, class FDeterministicRandom& Random);
    static FPlayerBoxScore& FindBox(TArray<FPlayerBoxScore>& BoxScore, const FGuid& PlayerId);
};
