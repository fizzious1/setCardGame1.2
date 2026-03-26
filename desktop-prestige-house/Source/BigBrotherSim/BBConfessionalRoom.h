// Copyright Big Brother AI Simulator. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBSimDataSubsystem.h"
#include "BBConfessionalRoom.generated.h"

class UCameraComponent;
class USpotLightComponent;
class UPointLightComponent;
class UWidgetComponent;
class ABBAgentCharacter;

UCLASS()
class BIGBROTHERSIM_API ABBConfessionalRoom : public AActor
{
	GENERATED_BODY()

public:
	ABBConfessionalRoom();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Start a confessional session for an agent */
	UFUNCTION(BlueprintCallable, Category = "BB|Confessional")
	void StartConfession(ABBAgentCharacter* Agent, const FBBGameEvent& ConfessionEvent);

	/** End the current confessional session */
	UFUNCTION(BlueprintCallable, Category = "BB|Confessional")
	void EndConfession();

	/** Check if confessional is active */
	UFUNCTION(BlueprintPure, Category = "BB|Confessional")
	bool IsConfessionActive() const { return bConfessionActive; }

	/** Get the confessional camera */
	UFUNCTION(BlueprintPure, Category = "BB|Confessional")
	UCameraComponent* GetConfessionalCamera() const { return ConfessionalCamera; }

	/** Get current confession text */
	UFUNCTION(BlueprintPure, Category = "BB|Confessional")
	FString GetConfessionText() const { return CurrentConfessionText; }

	/** Get the seated agent */
	UFUNCTION(BlueprintPure, Category = "BB|Confessional")
	ABBAgentCharacter* GetSeatedAgent() const;

	/** Handle event notification - auto trigger on confession events */
	UFUNCTION(BlueprintCallable, Category = "BB|Confessional")
	void OnEventOccurred(const FBBGameEvent& Event);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Components")
	USceneComponent* RootSceneComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Components")
	UCameraComponent* ConfessionalCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Components")
	USpotLightComponent* Spotlight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Components")
	UPointLightComponent* AmbientLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Components")
	UWidgetComponent* SubtitleWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Components")
	UWidgetComponent* MoodTrustOverlay;

	/** Position where the agent sits */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BB|Config")
	FVector ChairPosition = FVector(0.f, 0.f, 0.f);

	/** Rotation the agent faces when seated */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BB|Config")
	FRotator ChairRotation = FRotator(0.f, 180.f, 0.f);

	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	float SpotlightIntensity = 50000.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	float AmbientIntensityActive = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	float AmbientIntensityInactive = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	FLinearColor SpotlightColorActive = FLinearColor(1.f, 0.95f, 0.8f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	float ConfessionDisplayDuration = 5.0f;

private:
	void ActivateDramaticLighting();
	void DeactivateDramaticLighting();
	void UpdateSubtitleDisplay(float DeltaTime);

	TWeakObjectPtr<ABBAgentCharacter> SeatedAgent;
	bool bConfessionActive = false;
	FString CurrentConfessionText;
	float ConfessionTimer = 0.f;
	FBBGameEvent CurrentConfessionEvent;
};
