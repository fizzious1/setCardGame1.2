// Copyright Big Brother AI Simulator. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "BBPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ABBDirectorCamera;
class ABBFollowCamera;
class ABBAgentCharacter;
class ABBHUD;
enum class EBBCameraMode : uint8;

UCLASS()
class BIGBROTHERSIM_API ABBPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABBPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

	/** Get current camera mode */
	UFUNCTION(BlueprintPure, Category = "BB|Input")
	EBBCameraMode GetCurrentCameraMode() const;

	/** Get the director camera */
	UFUNCTION(BlueprintPure, Category = "BB|Input")
	ABBDirectorCamera* GetDirectorCamera() const { return DirectorCamera; }

	/** Get the follow camera */
	UFUNCTION(BlueprintPure, Category = "BB|Input")
	ABBFollowCamera* GetFollowCamera() const { return FollowCamera; }

protected:
	// Enhanced Input Actions
	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputAction* IA_CameraMove;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputAction* IA_CameraLook;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputAction* IA_CameraZoom;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputAction* IA_TogglePlayPause;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputAction* IA_StepForward;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputAction* IA_StepBackward;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputAction* IA_CameraDirector;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputAction* IA_CameraFollow;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputAction* IA_CameraOverview;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputAction* IA_CycleAgent;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputAction* IA_SelectAgent;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputAction* IA_ToggleReplay;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputAction* IA_Confessional;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputAction* IA_SpeedUp;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Input")
	UInputAction* IA_SlowDown;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	TSubclassOf<ABBDirectorCamera> DirectorCameraClass;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	TSubclassOf<ABBFollowCamera> FollowCameraClass;

private:
	// Input handlers
	void OnCameraMove(const FInputActionValue& Value);
	void OnCameraLook(const FInputActionValue& Value);
	void OnCameraZoom(const FInputActionValue& Value);
	void OnTogglePlayPause(const FInputActionValue& Value);
	void OnStepForward(const FInputActionValue& Value);
	void OnStepBackward(const FInputActionValue& Value);
	void OnCameraDirectorMode(const FInputActionValue& Value);
	void OnCameraFollowMode(const FInputActionValue& Value);
	void OnCameraOverviewMode(const FInputActionValue& Value);
	void OnCycleAgent(const FInputActionValue& Value);
	void OnSelectAgent(const FInputActionValue& Value);
	void OnToggleReplay(const FInputActionValue& Value);
	void OnConfessional(const FInputActionValue& Value);
	void OnSpeedUp(const FInputActionValue& Value);
	void OnSlowDown(const FInputActionValue& Value);

	void SpawnCameras();
	void SwitchToCameraMode(EBBCameraMode Mode);
	ABBAgentCharacter* TraceForAgent() const;

	UPROPERTY()
	ABBDirectorCamera* DirectorCamera;

	UPROPERTY()
	ABBFollowCamera* FollowCamera;

	EBBCameraMode CameraMode;
	bool bReplayMode = false;
};
