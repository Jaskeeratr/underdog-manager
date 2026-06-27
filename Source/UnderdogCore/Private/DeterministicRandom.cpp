#include "DeterministicRandom.h"

uint64 FDeterministicRandom::RotateLeft(uint64 Value, int32 Shift)
{
    return (Value << Shift) | (Value >> (64 - Shift));
}

uint64 FDeterministicRandom::SplitMix64(uint64& Value)
{
    uint64 Result = (Value += 0x9E3779B97F4A7C15ULL);
    Result = (Result ^ (Result >> 30)) * 0xBF58476D1CE4E5B9ULL;
    Result = (Result ^ (Result >> 27)) * 0x94D049BB133111EBULL;
    return Result ^ (Result >> 31);
}

FDeterministicRandom::FDeterministicRandom(uint64 Seed)
{
    uint64 Cursor = Seed == 0 ? 0x554E444552444F47ULL : Seed;
    for (uint64& Part : State) { Part = SplitMix64(Cursor); }
}

uint64 FDeterministicRandom::NextUInt64()
{
    const uint64 Result = RotateLeft(State[1] * 5, 7) * 9;
    const uint64 Temp = State[1] << 17;
    State[2] ^= State[0]; State[3] ^= State[1]; State[1] ^= State[2]; State[0] ^= State[3];
    State[2] ^= Temp; State[3] = RotateLeft(State[3], 45);
    return Result;
}

uint32 FDeterministicRandom::NextUInt32() { return static_cast<uint32>(NextUInt64() >> 32); }

int32 FDeterministicRandom::Range(int32 MinInclusive, int32 MaxInclusive)
{
    check(MaxInclusive >= MinInclusive);
    const uint64 Span = static_cast<uint64>(MaxInclusive) - MinInclusive + 1ULL;
    return MinInclusive + static_cast<int32>(NextUInt64() % Span);
}

bool FDeterministicRandom::ChancePerTenThousand(int32 Probability)
{
    return Range(1, 10000) <= FMath::Clamp(Probability, 0, 10000);
}
