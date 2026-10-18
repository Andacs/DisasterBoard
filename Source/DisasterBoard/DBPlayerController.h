#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DBPlayerController.generated.h"

class UInputMappingContext;
class ADBGameMode;

UCLASS()
class DISASTERBOARD_API ADBPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADBPlayerController();
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	int32 HoveredBuilding = -1;
	int32 HoveredDesk = -1;   // masadaki oyuncak (kupa, zar, ördek, lamba)
	int32 HoveredCar = -1;
	bool bHoveredCell = false;
	FIntPoint HoveredCell = FIntPoint(-1, -1);

	// Konsol (~)
	UFUNCTION(Exec) void DBTest();                 // akışı baştan sona otomatik oynar, ekran görüntüsü alır, kapanır
	// Ekip görsellerinin testi (komut satırı -DBVarlikTest, -DBVarlik="<aday klasörü>", -DBRapor="<dosya>"): her görsel
	// yuvasını gösterir, V_*.png ekran görüntüsü alır, hangi görselin yüklendiğini rapora yazar, kapanır (~35 sn)
	UFUNCTION(Exec) void DBVarlikTest();
	// Ses ve geri bildirim testi (komut satırı -DBSesTest): oyunun ses çıkışını kaydeder (Saved/DB_ses.wav), bilinen
	// anlarda tık / rüzgar / sayfa / tahta / su / doğru / yanlış tetikler, anları Saved/DB_ses_olaylar.txt'ye yazar,
	// rüzgar, dönüş ve fare izi görüntülerini (S_*.png) çeker, kapanır (~26 sn)
	UFUNCTION(Exec) void DBSesTest();
	UFUNCTION(Exec) void DBExposure(float EV100);  // sabit pozlama
	UFUNCTION(Exec) void DBTime(float Seconds);    // kalan süre
	UFUNCTION(Exec) void DBQuake();                // depremi şimdi başlat
	UFUNCTION(Exec) void DBPageGain(float Gain);   // takvim sayfası dokusunun parlaklığı
	UFUNCTION(Exec) void DBMusicWav();             // müziği 40 sn dosyaya yazar (yoğunluk 0→1): Saved/DB_muzik_test.wav
	// Tanıtım videosu çekimi (Tools/trailer_yap.sh, -benchmark -fps=30): akışı DBTest gibi GameMode çağrılarıyla sürer,
	// her kareyi REC_f_#####.png kaydeder, başlık / altyazı çizer, müziği Saved/DB_muzik_trailer.wav'a yazar, kapanır.
	// Yalnız çekim aracı: oynanışa dokunmaz.
	UFUNCTION(Exec) void DBSinema();
	// Esc / Eğitimi geç / Çıkış kontrolü (yalnız test): gerçek tıklama yolu, E_*.png, DB_ESCTEST satırları, kapanır
	UFUNCTION(Exec) void DBEscTest();
	// Rehber konuşma sesleri (Mert / Asil / Yiğit) dosyaya: Saved/DB_ses_karakter.wav, kapanır (yalnız ölçüm)
	UFUNCTION(Exec) void DBSesKarakter();

private:
	void OnClick();
	void OnBack();
	void OnRmb();   // sağ tık: geri (seçimi bırak, açık olanı kapat) ama duraklatma penceresini açmaz
	void OnRestart();
	void OnMusic();
	void OnZoom(const struct FInputActionValue& Value);
	void OnRotLeft();
	void OnRotRight();
	bool bMiniMouse = false;
	bool bLmbPrev = false;     // TESLİM: tuş bırakıldığı an bant normale döner   // mini oyunda basılı tutma Vazgeç düğmesinin dışında başladı
	bool CanControlBoard() const;
	ADBGameMode* GM() const;

	UPROPERTY() TObjectPtr<UInputMappingContext> Mapping;

	TArray<TFunction<void()>> TestSteps;
	int32 TestStep = 0;
	FTimerHandle TestTimer;
	void RunTestStep();
	void TestFab(int32 Kit, const TArray<int32>& Personal);   // fabrikada bir çantayı doğrudan hazırlar
	double SesT0 = 0.0;
	TArray<FString> SesOlaylar;
	void SesOlay(const FString& Ad);
	FTimerHandle CursorTimer;
	float CursorT = 0.f;
	void Shot(const FString& Name);
	// DBSinema
	bool bRecording = false;
	bool bRecPaused = false;    // kayıt duraklatıldı: oyun sürer, kare kaydedilmez (uzun ara kısımlar)
	int32 RecFrame = 0;
	float CinemaTime = 0.f;     // kaydedilen sürenin oyun zamanı
	float CaptionUntil = -1.f;
	float ClosingAt = -1.f;     // kapanış kartının başladığı an
	void Caption(const FString& Text, float Seconds);
	// Ham çekim (-DBSinemaHam): yazı / başlık çizilmez, sahne başları log'a yazılır (DB_SAHNE <ad> kare=N), kamera
	// yavaşça döner / yaklaşır; yazılar ve geçişler sonra kurgu betiğinde (Tools/trailer_kurgu.py) eklenir
	bool bHam = false;
	void Mark(const TCHAR* Name);
	void Cam(float Yaw, float Dist, float Pitch, float Seconds);   // kamerayı şimdiki yerinden hedefe yumuşakça götürür
	float CamT0 = 0.f, CamDur = 0.f;
	float CamYawA = 0.f, CamYawB = 0.f, CamDistA = 330.f, CamDistB = 330.f, CamPitchA = -50.f, CamPitchB = -50.f;
};
