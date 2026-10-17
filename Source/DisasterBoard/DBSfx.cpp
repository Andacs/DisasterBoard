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

	// İki kutuplu rezonatör (formant): konuşma sesindeki ağız boşluğu
	struct FReso
	{
		float Y1 = 0.f, Y2 = 0.f, A0 = 0.f, B1 = 0.f, B2 = 0.f;
		FReso(float F, float Bw)
		{
			const float R = FMath::Exp(-PI * Bw / SR);
			B1 = 2.f * R * FMath::Cos(2.f * PI * F / SR);
			B2 = -R * R;
			A0 = 1.f - R;
		}
		float Run(float X) { const float Y = A0 * X + B1 * Y1 + B2 * Y2; Y2 = Y1; Y1 = Y; return Y; }
	};

	// Hece: titreşen ses tellerinin darbe dizisi üç formanttan geçer, başta kısa ünsüz tıkı (Tomodachi Life'taki
	// sentez konuşma gibi). F0 hecenin içinde hafifçe kayar.
	void AddSyllable(TArray<float>& B, float Start, float Dur, float F1, float F2, float F3, float F0a, float F0b, float Amp, uint32 Seed)
	{
		FReso R1(F1, 90.f), R2(F2, 130.f), R3(F3, 200.f);
		FNoise N(Seed);
		FLowpass L;
		float Ph = 0.f;
		const int32 S0 = FMath::RoundToInt(Start * SR);
		const int32 Len = FMath::RoundToInt(Dur * SR);
		for (int32 k = 0; k < Len && S0 + k < B.Num(); ++k)
		{
			const float T = k / SR;
			const float U = T / Dur;
			const float F0 = FMath::Lerp(F0a, F0b, U);
			Ph += F0 / SR;
			Ph -= FMath::FloorToFloat(Ph);
			const float Glottal = (Ph < 0.4f ? FMath::Sin(PI * Ph / 0.4f) : 0.f) - 0.25f;   // yumuşak darbe
			const float Src = Glottal + 0.04f * N.Next();
			const float V = R1.Run(Src) * 1.f + R2.Run(Src) * 0.55f + R3.Run(Src) * 0.25f;
			const float E = FMath::Min(1.f, T / 0.012f) * FMath::Min(1.f, (Dur - T) / 0.035f);
			const float Cons = L.Run(N.Next(), 0.5f) * FMath::Exp(-T / 0.006f) * 0.25f;
			B[S0 + k] += (V * 3.f * E + Cons) * Amp;
		}
	}

	TArray<float> SynthVowel(float F1, float F2, float F3, uint32 Seed)
	{
		TArray<float> B = Make(0.12f);
		AddSyllable(B, 0.f, 0.115f, F1, F2, F3, 230.f, 205.f, 1.f, Seed);
		return B;
	}

	// Kasabalı selamı: "hı-hı?" iki hece, ikincisi yükselir
	TArray<float> SynthGreet()
	{
		TArray<float> B = Make(0.34f);
		AddSyllable(B, 0.f, 0.12f, 500.f, 1900.f, 2500.f, 210.f, 200.f, 1.f, 21);
		AddSyllable(B, 0.15f, 0.17f, 800.f, 1200.f, 2500.f, 205.f, 290.f, 1.f, 22);
		return B;
	}

	// Korna: iki kısa "bip", iki tonlu (400 + 500 Hz), kare dalga yumuşatılmış
	TArray<float> SynthHorn()
	{
		TArray<float> B = Make(0.42f);
		FLowpass L;
		for (int32 i = 0; i < B.Num(); ++i)
		{
			const float T = i / SR;
			const bool bOn = T < 0.14f || (T > 0.2f && T < 0.38f);
			const float Tl = T < 0.17f ? T : T - 0.2f;
			const float Lim = T < 0.17f ? 0.14f : 0.18f;
			const float E = bOn ? FMath::Min(1.f, Tl / 0.01f) * FMath::Min(1.f, (Lim - Tl) / 0.02f) : 0.f;
			const float Sq = (FMath::Sin(2.f * PI * 400.f * T) > 0.f ? 1.f : -1.f) + (FMath::Sin(2.f * PI * 500.f * T) > 0.f ? 1.f : -1.f);
			B[i] = L.Run(Sq, 0.18f) * E * 0.5f;
		}
		return B;
	}

	// Tarayıcı: yükselip inen radar süpürmesi, düzenli "bip"ler, altta uğultu
	TArray<float> SynthScan()
	{
		const float Dur = 2.9f;
		TArray<float> B = Make(Dur);
		float Ph = 0.f;
		for (int32 i = 0; i < B.Num(); ++i)
		{
			const float T = i / SR;
			const float U = T / Dur;
			const float Shape = FMath::Min(1.f, T / 0.15f) * FMath::Min(1.f, (Dur - T) / 0.4f);
			const float F = 520.f + 520.f * FMath::Sin(PI * U) + 60.f * FMath::Sin(T * 2.f * PI * 6.f);
			Ph += 2.f * PI * F / SR;
			const float Sweep = FMath::Sin(Ph) * 0.18f * (0.6f + 0.4f * FMath::Sin(T * 2.f * PI * 12.f));
			const float Hum = FMath::Sin(2.f * PI * 110.f * T) * 0.12f + FMath::Sin(2.f * PI * 220.f * T) * 0.05f;
			const float Tp = FMath::Fmod(T, 0.32f);
			const float Ping = FMath::Sin(2.f * PI * 1760.f * Tp) * FMath::Exp(-Tp / 0.05f) * 0.35f;
			B[i] = (Sweep + Hum + Ping) * Shape;
		}
		return B;
	}

	// Bant: döngü parçası (0,45 sn): motor uğultusu tam 40 periyot (dikişsiz) + üç makara tıkırtısı
	TArray<float> SynthBelt()
	{
		const float Dur = 0.45f;
		TArray<float> B = Make(Dur);
		FNoise N(31);
		FLowpass L;
		const float F = 40.f / Dur;
		for (int32 i = 0; i < B.Num(); ++i)
		{
			const float T = i / SR;
			const float Hum = FMath::Sin(2.f * PI * F * T) * 0.22f + FMath::Sin(4.f * PI * F * T) * 0.08f;
			const float Tc = FMath::Fmod(T, 0.15f);
			const float Clack = L.Run(N.Next(), 0.45f) * FMath::Exp(-Tc / 0.007f) * 0.6f
				+ FMath::Sin(2.f * PI * 900.f * Tc) * FMath::Exp(-Tc / 0.012f) * 0.15f;
			B[i] = Hum + Clack;
		}
		return B;
	}

	// Çanta: üç eşya içine düşer (tok, perdesi düşen), fermuar kapanır (hızlanan tıkırtı), sonda "çıt"
	TArray<float> SynthPack()
	{
		TArray<float> B = Make(0.9f);
		FNoise N(41);
		FLowpass L, Lz;
		for (int32 d = 0; d < 3; ++d)
		{
			const int32 S0 = FMath::RoundToInt((0.04f + d * 0.1f) * SR);
			float Ph = 0.f;
			for (int32 k = 0; k < (int32)(0.12f * SR) && S0 + k < B.Num(); ++k)
			{
				const float T = k / SR;
				Ph += 2.f * PI * (110.f + 140.f * FMath::Exp(-T / 0.03f) + d * 20.f) / SR;
				B[S0 + k] += FMath::Sin(Ph) * FMath::Exp(-T / 0.05f) * 0.7f + L.Run(N.Next(), 0.3f) * FMath::Exp(-T / 0.006f) * 0.4f;
			}
		}
		// Fermuar 0,38 → 0,72 sn: dişler giderek sıklaşır
		float Zp = 0.f;
		for (int32 i = FMath::RoundToInt(0.38f * SR); i < FMath::RoundToInt(0.72f * SR); ++i)
		{
			const float T = i / SR - 0.38f;
			Zp += (70.f + 260.f * T / 0.34f) / SR;
			const float Tooth = FMath::Exp(-(Zp - FMath::FloorToFloat(Zp)) * 40.f);
			const float Raw = N.Next();
			B[i] += (Raw - Lz.Run(Raw, 0.3f)) * Tooth * 0.9f;
		}
		for (int32 k = 0; k < (int32)(0.05f * SR); ++k)
		{
			const int32 I = FMath::RoundToInt(0.75f * SR) + k;
			const float T = k / SR;
			B[I] += (FMath::Sin(2.f * PI * 2400.f * T) * 0.5f + N.Next() * 0.4f) * FMath::Exp(-T / 0.006f);
		}
		return B;
	}

	// Başarım: hızlı yükselen arpej (Do-Mi-Sol-Do) + pırıltı
	TArray<float> SynthFanfare()
	{
		TArray<float> B = Make(1.2f);
		const float Notes[4] = { 1046.5f, 1318.5f, 1568.f, 2093.f };
		for (int32 n = 0; n < 4; ++n)
		{
			const int32 S0 = FMath::RoundToInt(n * 0.075f * SR);
			for (int32 k = 0; S0 + k < B.Num(); ++k)
			{
				const float T = k / SR;
				const float E = Env(T, 0.004f, n == 3 ? 0.45f : 0.2f);
				B[S0 + k] += (FMath::Sin(2.f * PI * Notes[n] * T) + 0.3f * FMath::Sin(4.f * PI * Notes[n] * T) + 0.1f * FMath::Sin(6.f * PI * Notes[n] * T)) * E * 0.3f;
			}
		}
		FNoise R(51);
		for (int32 s = 0; s < 10; ++s)
		{
			const int32 S0 = FMath::RoundToInt((0.3f + 0.06f * s) * SR);
			const float F = 3000.f + 1500.f * (R.Next() * 0.5f + 0.5f);
			for (int32 k = 0; k < (int32)(0.06f * SR) && S0 + k < B.Num(); ++k)
			{
				const float T = k / SR;
				B[S0 + k] += FMath::Sin(2.f * PI * F * T) * FMath::Exp(-T / 0.015f) * 0.08f;
			}
		}
		return B;
	}

	// Deklanşör: perde açılır-kapanır (iki kuru tık) + kısa mekanizma hışırtısı
	TArray<float> SynthShutter()
	{
		TArray<float> B = Make(0.2f);
		FNoise N(61);
		FLowpass L;
		for (int32 i = 0; i < B.Num(); ++i)
		{
			const float T = i / SR;
			const float Raw = N.Next();
			const float Hi = Raw - L.Run(Raw, 0.2f);
			const float C1 = FMath::Exp(-T / 0.004f);
			const float C2 = T > 0.07f ? FMath::Exp(-(T - 0.07f) / 0.005f) : 0.f;
			const float Whir = (T > 0.01f && T < 0.07f) ? 0.15f : 0.f;
			B[i] = Hi * (C1 + C2 * 0.8f + Whir) + FMath::Sin(2.f * PI * 1800.f * T) * C1 * 0.3f;
		}
		return B;
	}

	// Seramik kupa: uyumsuz kısmi tonlar (çan gibi), kısa vuruş
	TArray<float> SynthClink()
	{
		TArray<float> B = Make(0.7f);
		FNoise N(71);
		const float Fq[4] = { 2350.f, 3870.f, 5610.f, 1460.f };
		const float Dc[4] = { 0.22f, 0.12f, 0.07f, 0.3f };
		const float Am[4] = { 0.5f, 0.3f, 0.18f, 0.25f };
		for (int32 i = 0; i < B.Num(); ++i)
		{
			const float T = i / SR;
			float X = N.Next() * FMath::Exp(-T / 0.002f) * 0.5f;
			for (int32 k = 0; k < 4; ++k) X += FMath::Sin(2.f * PI * Fq[k] * T) * FMath::Exp(-T / Dc[k]) * Am[k];
			B[i] = X;
		}
		return B;
	}

	// Zar: masada sekerek yuvarlanır; giderek sıklaşan, sönen tık'lar
	TArray<float> SynthDice()
	{
		TArray<float> B = Make(0.75f);
		FNoise N(81);
		FLowpass L;
		float At = 0.f, Gap = 0.16f, Amp = 1.f;
		while (At < 0.7f)
		{
			const int32 S0 = FMath::RoundToInt(At * SR);
			const float F = 1500.f + 900.f * (N.Next() * 0.5f + 0.5f);
			for (int32 k = 0; k < (int32)(0.03f * SR) && S0 + k < B.Num(); ++k)
			{
				const float T = k / SR;
				B[S0 + k] += (L.Run(N.Next(), 0.6f) * FMath::Exp(-T / 0.003f) + FMath::Sin(2.f * PI * F * T) * FMath::Exp(-T / 0.008f) * 0.5f) * Amp;
			}
			At += Gap;
			Gap *= 0.72f;
			Amp *= 0.8f;
			if (Gap < 0.025f) break;
		}
		return B;
	}

	// Lastik ördek: perdesi yükselip inen vızıltılı ton, burundan (formant)
	TArray<float> SynthSqueak()
	{
		const float Dur = 0.32f;
		TArray<float> B = Make(Dur);
		FReso R1(1200.f, 300.f), R2(2600.f, 400.f);
		float Ph = 0.f;
		for (int32 i = 0; i < B.Num(); ++i)
		{
			const float T = i / SR;
			const float U = T / Dur;
			const float F = 700.f + 650.f * FMath::Sin(FMath::Min(1.f, U * 1.3f) * PI) + 40.f * FMath::Sin(T * 2.f * PI * 30.f);
			Ph += F / SR;
			Ph -= FMath::FloorToFloat(Ph);
			const float Src = Ph < 0.3f ? 1.f : -0.43f;
			const float E = FMath::Min(1.f, T / 0.015f) * FMath::Min(1.f, (Dur - T) / 0.05f);
			B[i] = (R1.Run(Src) + R2.Run(Src) * 0.6f) * E;
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
		// Ünlüler: F1, F2, F3 (Hz)
		Banks[(int32)EDBSfx::VoA] = SynthVowel(800.f, 1250.f, 2600.f, 11); Normalize(Banks[(int32)EDBSfx::VoA], 0.42f);
		Banks[(int32)EDBSfx::VoE] = SynthVowel(500.f, 1900.f, 2600.f, 12); Normalize(Banks[(int32)EDBSfx::VoE], 0.42f);
		Banks[(int32)EDBSfx::VoI] = SynthVowel(320.f, 2300.f, 3000.f, 13); Normalize(Banks[(int32)EDBSfx::VoI], 0.4f);
		Banks[(int32)EDBSfx::VoO] = SynthVowel(500.f, 880.f, 2450.f, 14);  Normalize(Banks[(int32)EDBSfx::VoO], 0.42f);
		Banks[(int32)EDBSfx::VoU] = SynthVowel(340.f, 800.f, 2300.f, 15);  Normalize(Banks[(int32)EDBSfx::VoU], 0.42f);
		Banks[(int32)EDBSfx::Greet] = SynthGreet();  Normalize(Banks[(int32)EDBSfx::Greet], 0.5f);
		Banks[(int32)EDBSfx::Horn] = SynthHorn();    Normalize(Banks[(int32)EDBSfx::Horn], 0.5f);
		Banks[(int32)EDBSfx::Scan] = SynthScan();    Normalize(Banks[(int32)EDBSfx::Scan], 0.45f);
		Banks[(int32)EDBSfx::Belt] = SynthBelt();    Normalize(Banks[(int32)EDBSfx::Belt], 0.4f);
		Banks[(int32)EDBSfx::Pack] = SynthPack();    Normalize(Banks[(int32)EDBSfx::Pack], 0.6f);
		Banks[(int32)EDBSfx::Fanfare] = SynthFanfare(); Normalize(Banks[(int32)EDBSfx::Fanfare], 0.55f);
		Banks[(int32)EDBSfx::Shutter] = SynthShutter(); Normalize(Banks[(int32)EDBSfx::Shutter], 0.55f);
		Banks[(int32)EDBSfx::Clink] = SynthClink();  Normalize(Banks[(int32)EDBSfx::Clink], 0.5f);
		Banks[(int32)EDBSfx::Dice] = SynthDice();    Normalize(Banks[(int32)EDBSfx::Dice], 0.55f);
		Banks[(int32)EDBSfx::Squeak] = SynthSqueak(); Normalize(Banks[(int32)EDBSfx::Squeak], 0.5f);
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
