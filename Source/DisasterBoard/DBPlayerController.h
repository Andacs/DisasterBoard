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
};
