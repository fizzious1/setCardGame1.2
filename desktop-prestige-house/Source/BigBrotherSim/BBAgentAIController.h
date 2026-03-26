// Copyright Big Brother AI Simulator. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BBSimDataSubsystem.h"
#include "BBAgentAIController.generated.h"

class ABBAgentCharacter;

UCLASS()
class BIGBROTHERSIM_API ABBAgentAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABBAgentAIController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** Command agent to move to a room */
	UFUNCTION(BlueprintCallable, Category = "BB|AI")
	void CommandMoveToRoom(const FString& RoomId);

	/** Command agent to move to a specific world location */
	UFUNCTION(BlueprintCallable, Category = "BB|AI")
	void CommandMoveToLocation(const FVector& Location);

	/** Apply a participant state update (from snapshot) */
	UFUNCTION(BlueprintCallable, Category = "BB|AI")
	void ApplyStateUpdate(const FBBParticipantState& NewState);

	/** Trigger a social action animation */
	UFUNCTION(BlueprintCallable, Category = "BB|AI")
	void TriggerSocialAction(const FString& AnimationHint);

	/** Get the controlled agent character */
	UFUNCTION(BlueprintPure, Category = "BB|AI")
	ABBAgentCharacter* GetAgentCharacter() const;

protected:
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	UPROPERTY(EditDefaultsOnly, Category = "BB|AI")
	float AcceptanceRadius = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "BB|AI")
	float PositionInterpSpeed = 4.f;

private:
	void InterpolateToTargetPosition(float DeltaTime);
	void HandleStateTransition(const FBBParticipantState& OldState, const FBBParticipantState& NewState);

	FBBParticipantState CurrentTargetState;
	FBBParticipantState PreviousState;
	FVector TargetWorldPosition = FVector::ZeroVector;
	bool bHasMoveCommand = false;
	bool bIsMoving = false;
};
