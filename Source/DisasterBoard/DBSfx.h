#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundWaveProcedural.h"
#include <atomic>
#include "DBSfx.generated.h"

// Dosyasız ses efektleri: hepsi koddan sentezlenir (asset yok, beyan: README → Ses, kendi üretimimiz).
// Tek bir stereo "karıştırıcı" ses: Play() kuyruğa ekler, ses kartı istedikçe çalan efektler karıştırılır
// (müzikle aynı yol, UDBMusicWave). Kesit: tık, rüzgar, sayfa, tahta kalkış/iniş, su sıçraması, doğru, yanlış.
enum class EDBSfx : uint8
{
	Click,     // düğme / tıklama: karton tık
	Wind,      // takvime yaklaşırken yandan rüzgar
	Page,      // takvim sayfası
	Lift,      // tahta masadan kalkar, döner (tahta gıcırtısı + uğultu)
	Land,      // tahta yerine iner (tok vuruş)
	Splash,    // denizden masaya su sıçrar
	Good,      // doğru: iki notalı çan
	Bad,       // yanlış: yumuşak vızıltı
	VoA,       // karakter sesi (Tomodachi Life gibi): sesli harf heceleri, perde konuşana göre
	VoE,
	VoI,
	VoO,
	VoU,
	Greet,     // kasabalı tıklanınca "Hm?"
	Horn,      // arabaya tıklayınca korna (bip bip)
	Scan,      // tarayıcı: radar süpürmesi + bip'ler
	Belt,      // fabrika bandı: makara tıkırtısı + motor uğultusu (döngü parçası)
	Pack,      // paket çantaya düşer (üç tok ses), fermuar kapanır
	Fanfare,   // başarım açıldı
	Shutter,   // fotoğraf makinesi deklanşörü (albüm)
	Count
};

UCLASS()
class DISASTERBOARD_API UDBSfxWave : public USoundWaveProcedural
{
	GENERATED_BODY()

public:
	UDBSfxWave(const FObjectInitializer& ObjectInitializer);
	virtual int32 OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples) override;

	// Pan: -1 sol .. 1 sağ; PanTo verilirse ses boyunca kayar (rüzgar yandan geçer). Pitch 1 = asıl perde.
	void Play(EDBSfx Id, float InVolume = 1.f, float Pan = 0.f, float InPitch = 1.f, float PanTo = -2.f);
	void SetMuted(bool b) { bMuted.store(b); }
	bool IsMuted() const { return bMuted.load(); }

	// Test: bütün efektleri sırayla dosyaya yazar (ölçmek / dinlemek için)
	static bool WriteTestWav(const FString& Path, FString* OutReport = nullptr);
	static float Length(EDBSfx Id);

	static constexpr int32 Rate = 44100;

private:
	struct FVoice
	{
		int32 Id = 0;
		double Pos = 0.0;
		float Step = 1.f;
		float Vol = 1.f;
		float Pan0 = 0.f;
		float Pan1 = 0.f;
	};
	FCriticalSection Lock;
	TArray<FVoice> Pending;   // oyun iş parçacığı ekler
	TArray<FVoice> Active;    // ses iş parçacığı çalar
	std::atomic<bool> bMuted{ false };

	static const TArray<float>& Bank(int32 Id);   // mono örnekler, ilk kullanımda üretilir
	void Mix(int16* Out, int32 Frames);
};
