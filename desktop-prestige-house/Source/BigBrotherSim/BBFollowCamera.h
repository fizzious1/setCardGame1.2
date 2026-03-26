// Copyright Big Brother AI Simulator. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBFollowCamera.generated.h"

class UCameraComponent;
class USpringArmComponent;
class ABBAgentCharacter;

UCLASS()
class BIGBROTHERSIM_API ABBFollowCamera : public AActor
{
	GENERATED_BODY()

public:
	ABBFollowCamera();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Set the target agent to follow */
	UFUNCTION(BlueprintCallable, Category = "BB|Camera")
	void SetFollowTarget(ABBAgentCharacter* NewTarget);

	/** Get current follow target */
	UFUNCTION(BlueprintPure, Category = "BB|Camera")
	ABBAgentCharacter* GetFollowTarget() const;

	/** Switch to next agent */
	UFUNCTION(BlueprintCallable, Category = "BB|Camera")
	void SwitchToNextTarget();

	/** Switch to previous agent */
	UFUNCTION(BlueprintCallable, Category = "BB|Camera")
	void SwitchToPreviousTarget();

	/** Enable orbit mode (rotate around character) */
	UFUNCTION(BlueprintCallable, Category = "BB|Camera")
	void SetOrbitMode(bool bEnable);

	/** Add orbit rotation input */
	UFUNCTION(BlueprintCallable, Category = "BB|Camera")
	void AddOrbitInput(float YawInput, float PitchInput);

	/** Get camera component */
	UFUNCTION(BlueprintPure, Category = "BB|Camera")
	UCameraComponent* GetCameraComponent() const { return CameraComp; }

	/** Is orbit mode active */
	UFUNCTION(BlueprintPure, Category = "BB|Camera")
	bool IsOrbitMode() const { return bOrbitMode; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Components")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Components")
	UCameraComponent* CameraComp;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Camera")
	float DefaultArmLength = 400.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Camera")
	float MinArmLength = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Camera")
	float MaxArmLength = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Camera")
	float FollowLagSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Camera")
	float OrbitSpeed = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Camera")
	float RoomSizeArmMultiplier = 0.5f;

private:
	void UpdateFollowPosition(float DeltaTime);
	void AdjustArmLengthForRoom();

	TWeakObjectPtr<ABBAgentCharacter> FollowTarget;
	bool bOrbitMode = false;
	float OrbitYaw = 0.f;
	float OrbitPitch = -20.f;
	int32 CurrentTargetIndex = 0;
};
