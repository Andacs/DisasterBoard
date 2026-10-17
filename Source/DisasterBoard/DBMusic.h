#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundWaveProcedural.h"
#include <atomic>
#include "DBMusic.generated.h"

// Dosyasız müzik: ses kartının istediği anda koddan üretilir (asset yok, beyan: README → Ses, kendi üretimimiz).
// Sakin, klasik havalı akor arpeji (Do – La minör – Fa – Sol). İlerleme (0..1) arttıkça tempo 72 → 112 BPM'e çıkar,
// önce bas, sonra melodi, sonra üst oktav pırıltıları girer.
UCLASS()
class DISASTERBOARD_API UDBMusicWave : public USoundWaveProcedural
{
	GENERATED_BODY()

public:
	UDBMusicWave(const FObjectInitializer& ObjectInitializer);
	virtual int32 OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples) override;

	void SetIntensity(float V) { Intensity.store(FMath::Clamp(V, 0.f, 1.f)); }
	void SetMuted(bool b) { bMuted.store(b); }
	bool IsMuted() const { return bMuted.load(); }
	float CurrentBpm() const { return Bpm; }

	// Test: aynı üreteci dosyaya yazar (yoğunluk 0'dan 1'e çıkar) → ölçmek için
	static bool WriteTestWav(const FString& Path, float Seconds, FString* OutReport = nullptr);

	static constexpr int32 Rate = 44100;

private:
	struct FVoice
	{
		float Freq = 0.f;
		float Phase = 0.f;
		float Amp = 0.f;
		float Decay = 1.f;   // sn
		float Age = 0.f;
		bool bOn = false;
	};
	FVoice Voices[24];
	double Step = 0.0;       // sekizlik nota sayacı
	int32 LastStep = -1;
	float Bpm = 72.f;
	float SmoothIntensity = 0.f;
	std::atomic<float> Intensity{ 0.f };
	std::atomic<bool> bMuted{ false };

	void Render(int16* Out, int32 NumSamples, float IntensityOverride = -1.f);
	void Trigger(int32 Midi, float Amp, float Decay);
	void OnStep(int32 S, float I);
};
