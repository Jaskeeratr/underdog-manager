#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundWaveProcedural.h"

class UAudioComponent;

class UNDERDOGGAME_API FProceduralAudioService
{
public:
	static USoundWaveProcedural* CreateCrowdAmbience(UObject* Outer);
	static USoundWaveProcedural* CreateBallBounce(UObject* Outer);
	static USoundWaveProcedural* CreateBuzzer(UObject* Outer);

private:
	static void FillBuffer(USoundWaveProcedural* Wave, TArray<uint8>& PCMData);
	static void GenerateNoise(TArray<int16>& Samples, int32 NumSamples, float Amplitude);
	static void GenerateSineBurst(TArray<int16>& Samples, int32 SampleRate, float Freq, float DurationSec, float Amplitude);
	static void GenerateTone(TArray<int16>& Samples, int32 SampleRate, float Freq, float DurationSec, float Amplitude);
};
