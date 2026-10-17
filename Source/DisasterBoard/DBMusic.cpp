#include "DBMusic.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"

namespace
{
	// Akor yürüyüşü (her ölçü 8 sekizlik): Do, La minör, Fa, Sol
	const int32 Roots[4] = { 48, 45, 41, 43 };
	const bool Minor[4] = { false, true, false, false };
	// Melodi: her ölçüde 4 dörtlük, akor seslerinden (kendi yazdığımız basit ezgi). -1 = sus
	const int32 Melody[16] = { 76, 79, 76, 74, 72, 76, 69, 72, 69, 72, 77, 76, 74, 71, 67, 71 };

	float MidiToHz(int32 M) { return 440.f * FMath::Pow(2.f, (M - 69) / 12.f); }
}

UDBMusicWave::UDBMusicWave(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetSampleRate(Rate);
	NumChannels = 1;
	Duration = INDEFINITELY_LOOPING_DURATION;
	bLooping = false;
	SoundGroup = SOUNDGROUP_Music;
}

void UDBMusicWave::Trigger(int32 Midi, float Amp, float Decay)
{
	int32 Best = 0;
	float BestAge = -1.f;
	for (int32 i = 0; i < UE_ARRAY_COUNT(Voices); ++i)
	{
		if (!Voices[i].bOn)
		{
			Best = i;
			break;
		}
		if (Voices[i].Age > BestAge)
		{
			BestAge = Voices[i].Age;
			Best = i;
		}
	}
	FVoice& V = Voices[Best];
	V.Freq = MidiToHz(Midi);
	V.Phase = 0.f;
	V.Amp = Amp;
	V.Decay = Decay;
	V.Age = 0.f;
	V.bOn = true;
}

void UDBMusicWave::OnStep(int32 S, float I)
{
	const int32 Bar = (S / 8) % 4;
	const int32 InBar = S % 8;
	const int32 Root = Roots[Bar];
	const int32 Third = Minor[Bar] ? 3 : 4;

	// Alberti arpeji (kök, beşli, üçlü, beşli) — hep çalar
	const int32 Pattern[4] = { 0, 7, Third, 7 };
	Trigger(Root + 12 + Pattern[InBar % 4], 0.16f, 1.4f);

	// Bas: ölçü başı; yoğunluk artınca ölçü ortasında beşli
	if (InBar == 0)
	{
		Trigger(Root, 0.24f, 2.6f);
	}
	if (InBar == 4 && I > 0.35f)
	{
		Trigger(Root + 7, 0.14f, 1.6f);
	}

	// Melodi dörtlüklerde (yoğunluk > 0,2)
	if (I > 0.2f && (InBar % 2) == 0)
	{
		const int32 Note = Melody[Bar * 4 + InBar / 2];
		if (Note > 0)
		{
			Trigger(Note, 0.11f + 0.05f * I, 0.9f);
		}
	}

	// Üst oktav pırıltı (yoğunluk > 0,65): ara sekizliklerde akorun üçlüsü
	if (I > 0.65f && (InBar % 2) == 1)
	{
		Trigger(Root + 24 + Third, 0.05f, 0.5f);
	}
}

void UDBMusicWave::Render(int16* Out, int32 NumSamples, float IntensityOverride)
{
	const float Dt = 1.f / Rate;
	const bool bMute = bMuted.load();
	for (int32 n = 0; n < NumSamples; ++n)
	{
		const float Target = IntensityOverride >= 0.f ? IntensityOverride : Intensity.load(std::memory_order_relaxed);
		SmoothIntensity += (Target - SmoothIntensity) * 0.00002f;  // ~1 sn'de yumuşak geçiş
		Bpm = FMath::Lerp(72.f, 112.f, SmoothIntensity);
		Step += Bpm / 60.f * 2.f * Dt;
		const int32 S = FMath::FloorToInt(Step);
		if (S != LastStep)
		{
			LastStep = S;
			OnStep(S, SmoothIntensity);
		}

		float Mix = 0.f;
		for (FVoice& V : Voices)
		{
			if (!V.bOn)
			{
				continue;
			}
			// Piyano / müzik kutusu: ana ses + iki harmonik, 6 ms atak, üstel sönüm
			const float Env = FMath::Min(1.f, V.Age / 0.006f) * FMath::Exp(-V.Age / V.Decay);
			const float P = V.Phase;
			Mix += V.Amp * Env * (FMath::Sin(P) + 0.32f * FMath::Sin(2.f * P) + 0.1f * FMath::Sin(3.f * P));
			V.Phase += 2.f * PI * V.Freq * Dt;
			if (V.Phase > 2.f * PI)
			{
				V.Phase -= 2.f * PI;
			}
			V.Age += Dt;
			if (V.Age > V.Decay * 6.f)
			{
				V.bOn = false;
			}
		}
		const float Y = bMute ? 0.f : FMath::Tanh(Mix * 0.9f) * 0.8f;
		Out[n] = (int16)FMath::Clamp(FMath::RoundToInt(Y * 32767.f), -32767, 32767);
	}
}

int32 UDBMusicWave::OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples)
{
	OutAudio.SetNumUninitialized(NumSamples * sizeof(int16));
	Render(reinterpret_cast<int16*>(OutAudio.GetData()), NumSamples);
	return NumSamples;
}

bool UDBMusicWave::WriteTestWav(const FString& Path, float Seconds, FString* OutReport)
{
	UDBMusicWave* W = NewObject<UDBMusicWave>();
	const int32 N = FMath::RoundToInt(Seconds * Rate);
	TArray<int16> Pcm;
	Pcm.SetNumZeroed(N);
	const int32 Block = 1024;
	int32 QuarterStart = 0;
	for (int32 i = 0; i < N; i += Block)
	{
		const float I = (float)i / N;  // yoğunluk 0 → 1
		W->Render(Pcm.GetData() + i, FMath::Min(Block, N - i), I);
		// Ölçüm: her çeyrekte kaç sekizlik çalındı → gerçek tempo
		const int32 Q = (i + Block) * 4 / N;
		if (OutReport && Q > 0 && (i * 4 / N) != Q)
		{
			const float Secs = Seconds / 4.f;
			const int32 Steps = W->LastStep - QuarterStart;
			*OutReport += FString::Printf(TEXT("[ceyrek %d: %d sekizlik / %.0f sn = %.0f BPM, anlik %.0f] "), Q, Steps, Secs, Steps / Secs * 30.f, W->Bpm);
			QuarterStart = W->LastStep;
		}
	}
	TArray<uint8> Wav;
	auto U32 = [&Wav](uint32 V) { Wav.Append(reinterpret_cast<uint8*>(&V), 4); };
	auto U16 = [&Wav](uint16 V) { Wav.Append(reinterpret_cast<uint8*>(&V), 2); };
	const uint32 DataBytes = N * 2;
	Wav.Append((const uint8*)"RIFF", 4); U32(36 + DataBytes); Wav.Append((const uint8*)"WAVEfmt ", 8);
	U32(16); U16(1); U16(1); U32(Rate); U32(Rate * 2); U16(2); U16(16);
	Wav.Append((const uint8*)"data", 4); U32(DataBytes);
	Wav.Append(reinterpret_cast<uint8*>(Pcm.GetData()), DataBytes);
	return FFileHelper::SaveArrayToFile(Wav, *Path);
}
