// Copyright Big Brother AI Simulator. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBSimDataSubsystem.h"
#include "BBDirectorCamera.generated.h"

class UCameraComponent;
class ABBAgentCharacter;
class ABBRoomVolume;

UENUM(BlueprintType)
enum class EBBShotType : uint8
{
	Wide		UMETA(DisplayName = "Wide Establishing"),
	Medium		UMETA(DisplayName = "Medium Shot"),
	CloseUp		UMETA(DisplayName = "Close-Up"),
	Overview	UMETA(DisplayName = "Overview")
};

UCLASS()
class BIGBROTHERSIM_API ABBDirectorCamera : public AActor
{
	GENERATED_BODY()

public:
	ABBDirectorCamera();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Get the camera component */
	UFUNCTION(BlueprintPure, Category = "BB|Camera")
	UCameraComponent* GetCameraComponent() const { return CameraComp; }

	/** Enable/disable auto-directing */
	UFUNCTION(BlueprintCallable, Category = "BB|Camera")
	void SetAutoDirecting(bool bEnable);

	/** Set manual override mode for WASD + mouse look */
	UFUNCTION(BlueprintCallable, Category = "BB|Camera")
	void SetManualOverride(bool bEnable);

	/** Is in manual override mode */
	UFUNCTION(BlueprintPure, Category = "BB|Camera")
	bool IsManualOverride() const { return bManualOverride; }

	/** Focus on a specific room */
	UFUNCTION(BlueprintCallable, Category = "BB|Camera")
	void FocusOnRoom(ABBRoomVolume* Room);

	/** Focus on a specific agent (close-up) */
	UFUNCTION(BlueprintCallable, Category = "BB|Camera")
	void FocusOnAgent(ABBAgentCharacter* Agent);

	/** Switch to overview mode */
	UFUNCTION(BlueprintCallable, Category = "BB|Camera")
	void SwitchToOverview(const FVector& OverviewPos, const FRotator& OverviewRot);

	/** Notify of a new event for reactive camera */
	UFUNCTION(BlueprintCallable, Category = "BB|Camera")
	void OnEventOccurred(const FBBGameEvent& Event);

	/** Manual input: move camera */
	UFUNCTION(BlueprintCallable, Category = "BB|Camera")
	void AddMovementInput(const FVector& Direction, float Scale);

	/** Manual input: look camera */
	UFUNCTION(BlueprintCallable, Category = "BB|Camera")
	void AddLookInput(float Yaw, float Pitch);

	/** Get current shot type */
	UFUNCTION(BlueprintPure, Category = "BB|Camera")
	EBBShotType GetCurrentShotType() const { return CurrentShotType; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Components")
	UCameraComponent* CameraComp;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Camera")
	float CutInterval = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Camera")
	float InterpSpeed = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Camera")
	float ManualMoveSpeed = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Camera")
	float ManualLookSpeed = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Camera")
	float CloseUpDistance = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Camera")
	float MediumDistance = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Camera")
	float WideDistance = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Camera")
	float DramaZoomMultiplier = 0.7f;

private:
	void UpdateAutoDirector(float DeltaTime);
	void InterpolateCameraTransform(float DeltaTime);
	void ChooseNextShot();
	void UpdateManualCamera(float DeltaTime);
	float ScoreRoom(ABBRoomVolume* Room) const;

	FVector TargetLocation;
	FRotator TargetRotation;
	FVector ManualMoveDelta;
	float ManualYawDelta = 0.f;
	float ManualPitchDelta = 0.f;

	EBBShotType CurrentShotType = EBBShotType::Wide;
	float TimeSinceLastCut = 0.f;
	bool bAutoDirecting = true;
	bool bManualOverride = false;

	TWeakObjectPtr<ABBRoomVolume> FocusedRoom;
	TWeakObjectPtr<ABBAgentCharacter> FocusedAgent;

	// Recent events for scoring
	TArray<FBBGameEvent> RecentEvents;
	float RecentEventClearTimer = 0.f;
};
