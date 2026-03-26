// Copyright Big Brother AI Simulator. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BBSimDataSubsystem.h"
#include "BBHUD.generated.h"

class ABBAgentCharacter;

UENUM(BlueprintType)
enum class EBBCameraMode : uint8
{
	Director	UMETA(DisplayName = "Director"),
	Follow		UMETA(DisplayName = "Follow"),
	Overview	UMETA(DisplayName = "Overview")
};

UCLASS()
class BIGBROTHERSIM_API ABBHUD : public AHUD
{
	GENERATED_BODY()

public:
	ABBHUD();

	virtual void DrawHUD() override;
	virtual void BeginPlay() override;

	/** Set the selected agent for info panel */
	UFUNCTION(BlueprintCallable, Category = "BB|HUD")
	void SetSelectedAgent(ABBAgentCharacter* Agent);

	/** Get the selected agent */
	UFUNCTION(BlueprintPure, Category = "BB|HUD")
	ABBAgentCharacter* GetSelectedAgent() const;

	/** Set camera mode indicator */
	UFUNCTION(BlueprintCallable, Category = "BB|HUD")
	void SetCameraMode(EBBCameraMode Mode);

	/** Show event popup */
	UFUNCTION(BlueprintCallable, Category = "BB|HUD")
	void ShowEventPopup(const FString& Text, const FLinearColor& Color);

	/** Set replay mode visible */
	UFUNCTION(BlueprintCallable, Category = "BB|HUD")
	void SetReplayControlsVisible(bool bVisible);

	/** Handle event for popup display */
	UFUNCTION()
	void OnEventTriggered(const FBBGameEvent& Event);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "BB|HUD")
	UFont* HUDFont;

	UPROPERTY(EditDefaultsOnly, Category = "BB|HUD")
	float MiniMapSize = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|HUD")
	float MiniMapPadding = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|HUD")
	float AgentDotSize = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|HUD")
	float EventPopupDuration = 4.0f;

private:
	void DrawDayHourDisplay();
	void DrawSelectedAgentInfo();
	void DrawMiniMap();
	void DrawCameraModeIndicator();
	void DrawEventPopup(float DeltaTime);
	void DrawReplayControls();
	void DrawActiveEventNotification();

	FVector2D WorldToMiniMap(const FVector& WorldPos) const;

	TWeakObjectPtr<ABBAgentCharacter> SelectedAgent;
	EBBCameraMode CurrentCameraMode = EBBCameraMode::Director;
	bool bReplayControlsVisible = false;

	// Event popup state
	FString PopupText;
	FLinearColor PopupColor = FLinearColor::White;
	float PopupTimer = 0.f;
	bool bPopupActive = false;

	// Mini-map bounds
	FVector2D MiniMapWorldMin = FVector2D(-2000.f, -2000.f);
	FVector2D MiniMapWorldMax = FVector2D(2000.f, 2000.f);
};
