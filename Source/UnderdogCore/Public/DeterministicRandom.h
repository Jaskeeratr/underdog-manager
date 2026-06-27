#pragma once

#include "CoreMinimal.h"

class UNDERDOGCORE_API FDeterministicRandom
{
public:
    explicit FDeterministicRandom(uint64 Seed);
    uint64 NextUInt64();
    uint32 NextUInt32();
    int32 Range(int32 MinInclusive, int32 MaxInclusive);
    bool ChancePerTenThousand(int32 Probability);
    static constexpr int32 Version = 1;

private:
    uint64 State[4];
    static uint64 SplitMix64(uint64& Value);
    static uint64 RotateLeft(uint64 Value, int32 Shift);
};
