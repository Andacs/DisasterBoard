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

	// Konsol (~)
	UFUNCTION(Exec) void DBTest();                 // akışı baştan sona otomatik oynar, ekran görüntüsü alır, kapanır
	UFUNCTION(Exec) void DBExposure(float EV100);  // sabit pozlama
	UFUNCTION(Exec) void DBTime(float Seconds);    // kalan süre
	UFUNCTION(Exec) void DBQuake();                // depremi şimdi başlat

private:
	void OnClick();
	void OnBack();
	void OnRestart();
	ADBGameMode* GM() const;

	UPROPERTY() TObjectPtr<UInputMappingContext> Mapping;

	TArray<TFunction<void()>> TestSteps;
	int32 TestStep = 0;
	FTimerHandle TestTimer;
	void RunTestStep();
	void Shot(const FString& Name);
};
