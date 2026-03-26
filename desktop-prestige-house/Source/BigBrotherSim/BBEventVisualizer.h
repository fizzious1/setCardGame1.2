// Copyright Big Brother AI Simulator. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBSimDataSubsystem.h"
#include "BBEventVisualizer.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;
class ABBAgentCharacter;
class ABBDirectorCamera;

USTRUCT(BlueprintType)
struct FBBEventVisualizationConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UNiagaraSystem* ParticleSystem = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	USoundBase* SoundCue = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FString AnimationHint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FString PopupText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FLinearColor PopupColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bCameraCloseUp = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bScreenShake = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float ScreenShakeIntensity = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bSlowMotion = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float SlowMotionDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float SlowMotionTimeDilation = 0.3f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEventPopup, const FString&, Text, const FLinearColor&, Color);

UCLASS()
class BIGBROTHERSIM_API ABBEventVisualizer : public AActor
{
	GENERATED_BODY()

public:
	ABBEventVisualizer();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Visualize a game event */
	UFUNCTION(BlueprintCallable, Category = "BB|Events")
	void VisualizeEvent(const FBBGameEvent& Event);

	/** Handle event from game mode */
	UFUNCTION(BlueprintCallable, Category = "BB|Events")
	void OnEventTriggered(const FBBGameEvent& Event);

	/** Get configuration for an event type */
	UFUNCTION(BlueprintPure, Category = "BB|Events")
	FBBEventVisualizationConfig GetConfigForEventType(EBBEventType EventType) const;

	UPROPERTY(BlueprintAssignable, Category = "BB|Events")
	FOnEventPopup OnEventPopup;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	TMap<EBBEventType, FBBEventVisualizationConfig> EventConfigs;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	float PopupDuration = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	TSubclassOf<UCameraShakeBase> DramaShake;

private:
	void VisualizeAlliance(const FBBGameEvent& Event, const FBBEventVisualizationConfig& Config);
	void VisualizeConflict(const FBBGameEvent& Event, const FBBEventVisualizationConfig& Config);
	void VisualizeBetrayal(const FBBGameEvent& Event, const FBBEventVisualizationConfig& Config);
	void VisualizeNomination(const FBBGameEvent& Event, const FBBEventVisualizationConfig& Config);
	void VisualizeEviction(const FBBGameEvent& Event, const FBBEventVisualizationConfig& Config);
	void VisualizeGenericEvent(const FBBGameEvent& Event, const FBBEventVisualizationConfig& Config);

	void FaceAgentsTowardEachOther(ABBAgentCharacter* A, ABBAgentCharacter* B);
	void TriggerCameraResponse(const FBBGameEvent& Event, const FBBEventVisualizationConfig& Config);
	void TriggerScreenShake(float Intensity);
	void BeginSlowMotion(float Duration, float TimeDilation);
	void EndSlowMotion();
	void SpawnParticleEffect(UNiagaraSystem* System, const FVector& Location, const FLinearColor& Color);
	void PlayEventSound(USoundBase* Sound, const FVector& Location);

	bool bSlowMotionActive = false;
	float SlowMotionTimer = 0.f;
	float SlowMotionDuration = 0.f;

	UPROPERTY()
	TArray<UNiagaraComponent*> ActiveParticles;
};
