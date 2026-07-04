#include "ProceduralAudioService.h"
#include "Sound/SoundWaveProcedural.h"

static constexpr int32 SampleRate = 22050;
static constexpr int32 NumChannels = 1;

void FProceduralAudioService::GenerateNoise(TArray<int16>& Samples, int32 NumSamples, float Amplitude)
{
	Samples.SetNum(NumSamples);
	FRandomStream Rng(42);
	int16 Prev = 0;
	for (int32 I = 0; I < NumSamples; ++I)
	{
		int16 Raw = static_cast<int16>(Rng.FRandRange(-Amplitude, Amplitude) * 32767.0f);
		// Low-pass filter for rumble effect
		Prev = static_cast<int16>((Prev * 7 + Raw) / 8);
		Samples[I] = Prev;
	}
}

void FProceduralAudioService::GenerateSineBurst(TArray<int16>& Samples, int32 Rate,
	float Freq, float DurationSec, float Amplitude)
{
	const int32 NumSamples = static_cast<int32>(Rate * DurationSec);
	Samples.SetNum(NumSamples);
	for (int32 I = 0; I < NumSamples; ++I)
	{
		const float T = static_cast<float>(I) / Rate;
		const float Envelope = 1.0f - (T / DurationSec);
		const float Value = FMath::Sin(2.0f * PI * Freq * T) * Amplitude * Envelope * Envelope;
		Samples[I] = static_cast<int16>(FMath::Clamp(Value, -1.0f, 1.0f) * 32767.0f);
	}
}

void FProceduralAudioService::GenerateTone(TArray<int16>& Samples, int32 Rate,
	float Freq, float DurationSec, float Amplitude)
{
	const int32 NumSamples = static_cast<int32>(Rate * DurationSec);
	Samples.SetNum(NumSamples);
	for (int32 I = 0; I < NumSamples; ++I)
	{
		const float T = static_cast<float>(I) / Rate;
		float Envelope = 1.0f;
		if (T > DurationSec - 0.05f) { Envelope = (DurationSec - T) / 0.05f; }
		if (T < 0.01f) { Envelope = T / 0.01f; }
		const float Value = FMath::Sin(2.0f * PI * Freq * T) * Amplitude * Envelope;
		Samples[I] = static_cast<int16>(FMath::Clamp(Value, -1.0f, 1.0f) * 32767.0f);
	}
}

void FProceduralAudioService::FillBuffer(USoundWaveProcedural* Wave, TArray<uint8>& PCMData)
{
	Wave->QueueAudio(PCMData.GetData(), PCMData.Num());
}

USoundWaveProcedural* FProceduralAudioService::CreateCrowdAmbience(UObject* Outer)
{
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(Outer);
	Wave->SetSampleRate(SampleRate);
	Wave->NumChannels = NumChannels;
	Wave->Duration = 4.0f;
	Wave->bLooping = true;
	Wave->Volume = 0.15f;

	TArray<int16> Samples;
	GenerateNoise(Samples, SampleRate * 4, 0.3f);

	TArray<uint8> PCMData;
	PCMData.SetNum(Samples.Num() * sizeof(int16));
	FMemory::Memcpy(PCMData.GetData(), Samples.GetData(), PCMData.Num());
	FillBuffer(Wave, PCMData);

	return Wave;
}

USoundWaveProcedural* FProceduralAudioService::CreateBallBounce(UObject* Outer)
{
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(Outer);
	Wave->SetSampleRate(SampleRate);
	Wave->NumChannels = NumChannels;
	Wave->Duration = 0.12f;
	Wave->bLooping = false;
	Wave->Volume = 0.5f;

	TArray<int16> Samples;
	GenerateSineBurst(Samples, SampleRate, 180.0f, 0.12f, 0.8f);

	TArray<uint8> PCMData;
	PCMData.SetNum(Samples.Num() * sizeof(int16));
	FMemory::Memcpy(PCMData.GetData(), Samples.GetData(), PCMData.Num());
	FillBuffer(Wave, PCMData);

	return Wave;
}

USoundWaveProcedural* FProceduralAudioService::CreateBuzzer(UObject* Outer)
{
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(Outer);
	Wave->SetSampleRate(SampleRate);
	Wave->NumChannels = NumChannels;
	Wave->Duration = 1.5f;
	Wave->bLooping = false;
	Wave->Volume = 0.35f;

	TArray<int16> Samples;
	GenerateTone(Samples, SampleRate, 440.0f, 1.5f, 0.6f);

	TArray<uint8> PCMData;
	PCMData.SetNum(Samples.Num() * sizeof(int16));
	FMemory::Memcpy(PCMData.GetData(), Samples.GetData(), PCMData.Num());
	FillBuffer(Wave, PCMData);

	return Wave;
}
