// Copyright Big Brother AI Simulator. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BBSimDataSubsystem.h"
#include "BBAgentCharacter.generated.h"

class UWidgetComponent;

UENUM(BlueprintType)
enum class EBBAgentState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Walking		UMETA(DisplayName = "Walking"),
	SocialAction	UMETA(DisplayName = "Social Action"),
	Confessing	UMETA(DisplayName = "Confessing")
};

UCLASS()
class BIGBROTHERSIM_API ABBAgentCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABBAgentCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Initialize with participant base data */
	UFUNCTION(BlueprintCallable, Category = "BB|Agent")
	void InitializeParticipant(const FBBParticipant& InParticipant);

	/** Apply state from simulation snapshot */
	UFUNCTION(BlueprintCallable, Category = "BB|Agent")
	void UpdateFromSnapshot(const FBBParticipantState& State);

	/** Get participant ID */
	UFUNCTION(BlueprintPure, Category = "BB|Agent")
	FString GetParticipantId() const { return ParticipantData.ParticipantId; }

	/** Get display name */
	UFUNCTION(BlueprintPure, Category = "BB|Agent")
	FString GetDisplayName() const { return ParticipantData.DisplayName; }

	/** Get current mood */
	UFUNCTION(BlueprintPure, Category = "BB|Agent")
	EBBMood GetCurrentMood() const { return CurrentState.Mood; }

	/** Get current room ID */
	UFUNCTION(BlueprintPure, Category = "BB|Agent")
	FString GetCurrentRoomId() const { return CurrentState.CurrentRoomId; }

	/** Get agent state machine state */
	UFUNCTION(BlueprintPure, Category = "BB|Agent")
	EBBAgentState GetAgentState() const { return AgentState; }

	/** Get current participant state */
	UFUNCTION(BlueprintPure, Category = "BB|Agent")
	const FBBParticipantState& GetCurrentParticipantState() const { return CurrentState; }

	/** Get base participant data */
	UFUNCTION(BlueprintPure, Category = "BB|Agent")
	const FBBParticipant& GetParticipantData() const { return ParticipantData; }

	/** Move to a target location using navigation */
	UFUNCTION(BlueprintCallable, Category = "BB|Agent")
	void MoveToLocation(const FVector& TargetLocation);

	/** Play a social action animation by hint name */
	UFUNCTION(BlueprintCallable, Category = "BB|Agent")
	void PlaySocialAnimation(const FString& AnimationHint);

	/** Set agent state machine state */
	UFUNCTION(BlueprintCallable, Category = "BB|Agent")
	void SetAgentState(EBBAgentState NewState);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Components")
	UWidgetComponent* NameTagWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Components")
	UWidgetComponent* MoodIndicatorWidget;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Animation")
	TMap<FString, UAnimMontage*> SocialAnimations;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	float MoveSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	float PositionInterpSpeed = 5.f;

private:
	void UpdateNameTag();
	void UpdateMoodIndicator();
	void InterpolateToTargetPosition(float DeltaTime);
	FLinearColor GetMoodColor() const;

	FBBParticipant ParticipantData;
	FBBParticipantState CurrentState;
	EBBAgentState AgentState = EBBAgentState::Idle;

	FVector TargetPosition = FVector::ZeroVector;
	FRotator TargetRotation = FRotator::ZeroRotator;
	bool bHasTargetPosition = false;
};
