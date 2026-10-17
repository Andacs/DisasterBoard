#include "DBSfx.h"
#include "Misc/FileHelper.h"

namespace
{
	constexpr float SR = (float)UDBSfxWave::Rate;

	// Basit, tekrarlanabilir gürültü (her efekt kendi tohumuyla)
	struct FNoise
	{
		uint32 S;
		explicit FNoise(uint32 Seed) : S(Seed * 2654435761u + 1u) {}
		float Next() { S ^= S << 13; S ^= S >> 17; S ^= S << 5; return (S & 0xFFFFFF) / 8388608.f - 1.f; }
	};

	// Tek kutuplu alçak geçiren süzgeç: A küçüldükçe ses koyulaşır
	struct FLowpass
	{
		float Y = 0.f;
		float Run(float X, float A) { Y += A * (X - Y); return Y; }
	};

	float Env(float T, float Attack, float Decay)
	{
		return T < Attack ? T / Attack : FMath::Exp(-(T - Attack) / Decay);
	}

	TArray<float> Make(float Seconds)
	{
		TArray<float> B;
		B.SetNumZeroed(FMath::RoundToInt(Seconds * SR));
		return B;
	}

	// Karton tık: kısa gürültü vuruşu + iki tok ton
	TArray<float> SynthClick()
	{
		TArray<float> B = Make(0.11f);
		FNoise N(1);
		FLowpass L;
		for (int32 i = 0; i < B.Num(); ++i)
		{
			const float T = i / SR;
			const float Tap = L.Run(N.Next(), 0.35f) * FMath::Exp(-T / 0.006f) * 0.9f;
			const float Body = FMath::Sin(2.f * PI * 1150.f * T) * FMath::Exp(-T / 0.018f) * 0.35f
				+ FMath::Sin(2.f * PI * 620.f * T) * FMath::Exp(-T / 0.035f) * 0.3f;
			B[i] = Tap + Body;
		}
		return B;
	}

	// Rüzgar: koyu gürültü, süzgeci açılıp kapanan bir esinti; ortada şişer
	TArray<float> SynthWind()
	{
		TArray<float> B = Make(1.6f);
		FNoise N(2);
		FLowpass L1, L2;
		for (int32 i = 0; i < B.Num(); ++i)
		{
			const float T = i / SR;
			const float U = T / 1.6f;
			const float Shape = FMath::Pow(FMath::Sin(PI * U), 1.6f);
			const float A = 0.015f + 0.07f * Shape + 0.02f * FMath::Sin(T * 9.f) * Shape;
			const float X = L2.Run(L1.Run(N.Next(), A), A * 1.4f);
			B[i] = X * Shape * 2.6f;
		}
		return B;
	}

	// Kâğıt: yüksek frekanslı hışırtı, iki kısa kıpırtı
	TArray<float> SynthPage()
	{
		TArray<float> B = Make(0.45f);
		FNoise N(3);
		FLowpass L;
		for (int32 i = 0; i < B.Num(); ++i)
		{
			const float T = i / SR;
			const float Raw = N.Next();
			const float High = Raw - L.Run(Raw, 0.25f);   // yüksek geçiren
			const float E = Env(T, 0.03f, 0.08f) * 0.8f + Env(FMath::Max(0.f, T - 0.16f), 0.02f, 0.09f) * (T > 0.16f ? 0.6f : 0.f);
			B[i] = High * E * (0.7f + 0.3f * FMath::Sin(T * 140.f));
		}
		return B;
	}

	// Tahta kalkıyor: alçak uğultu yükselir, tahta gıcırtısı (titreşen ton), üstüne esinti
	TArray<float> SynthLift()
	{
		TArray<float> B = Make(0.95f);
		FNoise N(4);
		FLowpass L;
		float Ph = 0.f;
		for (int32 i = 0; i < B.Num(); ++i)
		{
			const float T = i / SR;
			const float U = T / 0.95f;
			const float Shape = FMath::Sin(PI * U);
			const float F = 140.f + 60.f * U + 18.f * FMath::Sin(T * 37.f);
			Ph += 2.f * PI * F / SR;
			const float Creak = (FMath::Sin(Ph) + 0.4f * FMath::Sin(2.f * Ph) + 0.2f * FMath::Sin(3.f * Ph)) * 0.16f * Shape
				* (0.6f + 0.4f * FMath::Sin(T * 23.f));
			const float Air = L.Run(N.Next(), 0.03f + 0.05f * Shape) * Shape * 1.6f;
			B[i] = Creak + Air;
		}
		return B;
	}

	// Tahta iniyor: perdesi düşen tok ses + vuruş tıkırtısı + kısa tahta tını
	TArray<float> SynthLand()
	{
		TArray<float> B = Make(0.45f);
		FNoise N(5);
		FLowpass L;
		float Ph = 0.f;
		for (int32 i = 0; i < B.Num(); ++i)
		{
			const float T = i / SR;
			const float F = 60.f + 70.f * FMath::Exp(-T / 0.05f);
			Ph += 2.f * PI * F / SR;
			const float Thump = FMath::Sin(Ph) * FMath::Exp(-T / 0.09f) * 0.9f;
			const float Hit = L.Run(N.Next(), 0.4f) * FMath::Exp(-T / 0.008f) * 0.7f;
			const float Wood = FMath::Sin(2.f * PI * 240.f * T) * FMath::Exp(-T / 0.06f) * 0.25f;
			B[i] = Thump + Hit + Wood;
		}
		return B;
	}

	// Su: yumuşak şıpırtı gürültüsü + rastgele damla "plip"leri (perdesi hızla düşen tonlar)
	TArray<float> SynthSplash()
	{
		TArray<float> B = Make(1.1f);
		FNoise N(6);
		FLowpass L;
		for (int32 i = 0; i < B.Num(); ++i)
		{
			const float T = i / SR;
			B[i] = L.Run(N.Next(), 0.12f) * Env(T, 0.02f, 0.25f) * 0.55f;
		}
		FNoise R(7);
		for (int32 d = 0; d < 14; ++d)
		{
			const float Start = (R.Next() * 0.5f + 0.5f) * 0.8f;
			const float F0 = 900.f + (R.Next() * 0.5f + 0.5f) * 1100.f;
			const float Amp = 0.18f + 0.12f * (R.Next() * 0.5f + 0.5f);
			float Ph = 0.f;
			const int32 S0 = FMath::RoundToInt(Start * SR);
			for (int32 k = 0; k < (int32)(0.05f * SR) && S0 + k < B.Num(); ++k)
			{
				const float T = k / SR;
				Ph += 2.f * PI * F0 * (1.f + 1.4f * FMath::Exp(-T / 0.006f)) / SR;
				B[S0 + k] += FMath::Sin(Ph) * FMath::Exp(-T / 0.012f) * Amp;
			}
		}
		return B;
	}

	// Doğru: iki notalı çan (Mi6, Si6), hafif üst harmonik
	TArray<float> SynthGood()
	{
		TArray<float> B = Make(0.9f);
		const float Notes[2] = { 1318.5f, 1975.5f };
		for (int32 n = 0; n < 2; ++n)
		{
			const int32 S0 = FMath::RoundToInt(n * 0.11f * SR);
			for (int32 k = 0; S0 + k < B.Num(); ++k)
			{
				const float T = k / SR;
				const float E = Env(T, 0.004f, 0.28f);
				B[S0 + k] += (FMath::Sin(2.f * PI * Notes[n] * T) + 0.25f * FMath::Sin(4.f * PI * Notes[n] * T)) * E * 0.32f;
			}
		}
		return B;
	}

	// Yanlış: iki kısa, alçak, yumuşak vızıltı (tek harmonikler)
	TArray<float> SynthBad()
	{
		TArray<float> B = Make(0.42f);
		for (int32 i = 0; i < B.Num(); ++i)
		{
			const float T = i / SR;
			const float Pulse = (T < 0.14f) ? Env(T, 0.01f, 0.08f) : (T > 0.19f ? Env(T - 0.19f, 0.01f, 0.09f) : 0.f);
			const float F = T < 0.17f ? 196.f : 165.f;
			float X = 0.f;
			for (int32 h = 1; h <= 7; h += 2)
			{
				X += FMath::Sin(2.f * PI * F * h * T) / h;
			}
			B[i] = X * Pulse * 0.33f;
		}
		return B;
	}

	void Normalize(TArray<float>& B, float Peak)
	{
		float M = 0.f;
		for (float V : B) M = FMath::Max(M, FMath::Abs(V));
		if (M > 0.f)
		{
			for (float& V : B) V *= Peak / M;
		}
	}
}

UDBSfxWave::UDBSfxWave(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetSampleRate(Rate);
	NumChannels = 2;
	Duration = INDEFINITELY_LOOPING_DURATION;
	bLooping = false;
	SoundGroup = SOUNDGROUP_Effects;
}

const TArray<float>& UDBSfxWave::Bank(int32 Id)
{
	static TArray<float> Banks[(int32)EDBSfx::Count];
	static bool bBuilt = false;
	if (!bBuilt)
	{
		// Tepe seviyeleri oyunun kaydedilen çıkışıyla ölçülerek ayarlandı (DBSesTest): ilk hâlde müziğin ancak 2-4 dB üstündeydi
		Banks[(int32)EDBSfx::Click] = SynthClick();  Normalize(Banks[(int32)EDBSfx::Click], 0.6f);
		Banks[(int32)EDBSfx::Wind] = SynthWind();    Normalize(Banks[(int32)EDBSfx::Wind], 0.55f);
		Banks[(int32)EDBSfx::Page] = SynthPage();    Normalize(Banks[(int32)EDBSfx::Page], 0.5f);
		Banks[(int32)EDBSfx::Lift] = SynthLift();    Normalize(Banks[(int32)EDBSfx::Lift], 0.65f);
		Banks[(int32)EDBSfx::Land] = SynthLand();    Normalize(Banks[(int32)EDBSfx::Land], 0.85f);
		Banks[(int32)EDBSfx::Splash] = SynthSplash(); Normalize(Banks[(int32)EDBSfx::Splash], 0.6f);
		Banks[(int32)EDBSfx::Good] = SynthGood();    Normalize(Banks[(int32)EDBSfx::Good], 0.55f);
		Banks[(int32)EDBSfx::Bad] = SynthBad();      Normalize(Banks[(int32)EDBSfx::Bad], 0.5f);
		bBuilt = true;
	}
	return Banks[FMath::Clamp(Id, 0, (int32)EDBSfx::Count - 1)];
}

float UDBSfxWave::Length(EDBSfx Id)
{
	return Bank((int32)Id).Num() / SR;
}

void UDBSfxWave::Play(EDBSfx Id, float InVolume, float Pan, float InPitch, float PanTo)
{
	Bank((int32)Id);   // ilk çağrıda oyun iş parçacığında üret (ses iş parçacığı hazır bulsun)
	FVoice V;
	V.Id = (int32)Id;
	V.Step = FMath::Clamp(InPitch, 0.25f, 4.f);
	V.Vol = InVolume;
	V.Pan0 = FMath::Clamp(Pan, -1.f, 1.f);
	V.Pan1 = PanTo < -1.5f ? V.Pan0 : FMath::Clamp(PanTo, -1.f, 1.f);
	FScopeLock L(&Lock);
	Pending.Add(V);
}

void UDBSfxWave::Mix(int16* Out, int32 Frames)
{
	{
		FScopeLock L(&Lock);
		Active.Append(Pending);
		Pending.Reset();
	}
	const bool bMute = bMuted.load();
	for (int32 f = 0; f < Frames; ++f)
	{
		float Lft = 0.f, Rgt = 0.f;
		for (FVoice& V : Active)
		{
			const TArray<float>& B = Bank(V.Id);
			const int32 I = (int32)V.Pos;
			if (I + 1 >= B.Num())
			{
				continue;
			}
			const float Fr = (float)(V.Pos - I);
			const float S = FMath::Lerp(B[I], B[I + 1], Fr) * V.Vol;
			const float P = FMath::Lerp(V.Pan0, V.Pan1, (float)(V.Pos / B.Num()));
			// Eşit güçte pan
			const float A = (P + 1.f) * 0.25f * PI;
			Lft += S * FMath::Cos(A);
			Rgt += S * FMath::Sin(A);
			V.Pos += V.Step;
		}
		if (bMute)
		{
			Lft = Rgt = 0.f;
		}
		Out[2 * f] = (int16)FMath::Clamp(FMath::RoundToInt(FMath::Clamp(Lft, -1.f, 1.f) * 32000.f), -32767, 32767);
		Out[2 * f + 1] = (int16)FMath::Clamp(FMath::RoundToInt(FMath::Clamp(Rgt, -1.f, 1.f) * 32000.f), -32767, 32767);
	}
	Active.RemoveAll([](const FVoice& V) { return (int32)V.Pos + 1 >= Bank(V.Id).Num(); });
}

int32 UDBSfxWave::OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples)
{
	// NumSamples: kanallar dahil toplam örnek (stereo: kare × 2)
	const int32 Frames = NumSamples / 2;
	OutAudio.SetNumUninitialized(Frames * 2 * sizeof(int16));
	Mix(reinterpret_cast<int16*>(OutAudio.GetData()), Frames);
	return Frames * 2;
}

bool UDBSfxWave::WriteTestWav(const FString& Path, FString* OutReport)
{
	UDBSfxWave* W = NewObject<UDBSfxWave>();
	TArray<int16> Pcm;
	float T = 0.f;
	for (int32 i = 0; i < (int32)EDBSfx::Count; ++i)
	{
		W->Play((EDBSfx)i, 1.f, i == (int32)EDBSfx::Wind ? -0.9f : 0.f, 1.f, i == (int32)EDBSfx::Wind ? 0.9f : -2.f);
		const int32 Frames = FMath::RoundToInt((Length((EDBSfx)i) + 0.25f) * SR);
		const int32 Base = Pcm.Num();
		Pcm.AddZeroed(Frames * 2);
		W->Mix(Pcm.GetData() + Base, Frames);
		if (OutReport)
		{
			int32 Peak = 0;
			double Sum = 0.0;
			for (int32 k = Base; k < Pcm.Num(); ++k)
			{
				Peak = FMath::Max(Peak, FMath::Abs((int32)Pcm[k]));
				Sum += (double)Pcm[k] * Pcm[k];
			}
			*OutReport += FString::Printf(TEXT("[%d @%.2fs tepe %.1f dBFS rms %.1f dBFS] "), i, T,
				20.f * FMath::LogX(10.f, FMath::Max(1, Peak) / 32767.f), 10.f * FMath::LogX(10.f, (float)(Sum / (Pcm.Num() - Base)) / (32767.f * 32767.f) + 1e-12f));
		}
		T += Frames / SR;
	}
	// WAV başlığı (16 bit stereo)
	TArray<uint8> Wav;
	auto U32 = [&Wav](uint32 V) { Wav.Append(reinterpret_cast<uint8*>(&V), 4); };
	auto U16 = [&Wav](uint16 V) { Wav.Append(reinterpret_cast<uint8*>(&V), 2); };
	const uint32 DataBytes = Pcm.Num() * 2;
	Wav.Append((const uint8*)"RIFF", 4); U32(36 + DataBytes); Wav.Append((const uint8*)"WAVE", 4);
	Wav.Append((const uint8*)"fmt ", 4); U32(16); U16(1); U16(2); U32(Rate); U32(Rate * 4); U16(4); U16(16);
	Wav.Append((const uint8*)"data", 4); U32(DataBytes);
	Wav.Append(reinterpret_cast<const uint8*>(Pcm.GetData()), DataBytes);
	return FFileHelper::SaveArrayToFile(Wav, *Path);
}
