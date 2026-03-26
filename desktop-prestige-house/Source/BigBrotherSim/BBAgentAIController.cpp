// Copyright Big Brother AI Simulator. All Rights Reserved.

#include "BBAgentAIController.h"
#include "BigBrotherSim.h"
#include "BBAgentCharacter.h"
#include "BBGameMode.h"
#include "BBHouseFloor.h"
#include "BBRoomVolume.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Kismet/GameplayStatics.h"

ABBAgentAIController::ABBAgentAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABBAgentAIController::BeginPlay()
{
	Super::BeginPlay();
}

void ABBAgentAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bHasMoveCommand && !bIsMoving)
	{
		InterpolateToTargetPosition(DeltaTime);
	}
}

void ABBAgentAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ABBAgentCharacter* Agent = Cast<ABBAgentCharacter>(InPawn);
	if (Agent)
	{
		UE_LOG(LogBigBrotherSim, Verbose, TEXT("AI Controller possessed: %s"), *Agent->GetDisplayName());
	}
}

void ABBAgentAIController::OnUnPossess()
{
	bHasMoveCommand = false;
	bIsMoving = false;
	Super::OnUnPossess();
}

void ABBAgentAIController::CommandMoveToRoom(const FString& RoomId)
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM || !GM->GetHouseFloor())
	{
		return;
	}

	ABBRoomVolume* Room = GM->GetHouseFloor()->GetRoomById(RoomId);
	if (!Room)
	{
		UE_LOG(LogBigBrotherSim, Warning, TEXT("AI: Room not found: %s"), *RoomId);
		return;
	}

	FVector NavPoint = Room->GetRandomPositionInRoom();
	CommandMoveToLocation(NavPoint);
}

void ABBAgentAIController::CommandMoveToLocation(const FVector& Location)
{
	TargetWorldPosition = Location;
	bHasMoveCommand = true;

	ABBAgentCharacter* Agent = GetAgentCharacter();
	if (Agent)
	{
		Agent->SetAgentState(EBBAgentState::Walking);
	}

	// Try nav mesh path first
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		FAIMoveRequest MoveReq(Location);
		MoveReq.SetAcceptanceRadius(AcceptanceRadius);
		MoveReq.SetUsePathfinding(true);

		FPathFollowingRequestResult Result = MoveTo(MoveReq);

		if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
		{
			bIsMoving = true;
			return;
		}
	}

	// Fallback to direct interpolation
	bIsMoving = false;
	UE_LOG(LogBigBrotherSim, Verbose, TEXT("AI: Using direct interpolation for movement (no nav mesh path)"));
}

void ABBAgentAIController::ApplyStateUpdate(const FBBParticipantState& NewState)
{
	PreviousState = CurrentTargetState;
	CurrentTargetState = NewState;

	ABBAgentCharacter* Agent = GetAgentCharacter();
	if (!Agent)
	{
		return;
	}

	HandleStateTransition(PreviousState, NewState);

	// Move to new room if changed
	if (PreviousState.CurrentRoomId != NewState.CurrentRoomId && !NewState.CurrentRoomId.IsEmpty())
	{
		CommandMoveToRoom(NewState.CurrentRoomId);
	}
	else if (!NewState.WorldPosition.IsNearlyZero())
	{
		// Update target position for interpolation
		TargetWorldPosition = NewState.WorldPosition;
		bHasMoveCommand = true;
	}

	// Trigger animation if there's an action hint
	if (!NewState.CurrentAction.IsEmpty() &&
		!NewState.CurrentAction.Equals(TEXT("idle"), ESearchCase::IgnoreCase) &&
		NewState.CurrentAction != PreviousState.CurrentAction)
	{
		TriggerSocialAction(NewState.CurrentAction);
	}
}

void ABBAgentAIController::TriggerSocialAction(const FString& AnimationHint)
{
	ABBAgentCharacter* Agent = GetAgentCharacter();
	if (Agent)
	{
		Agent->PlaySocialAnimation(AnimationHint);
	}
}

ABBAgentCharacter* ABBAgentAIController::GetAgentCharacter() const
{
	return Cast<ABBAgentCharacter>(GetPawn());
}

void ABBAgentAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	bIsMoving = false;
	bHasMoveCommand = false;

	ABBAgentCharacter* Agent = GetAgentCharacter();
	if (Agent && Agent->GetAgentState() == EBBAgentState::Walking)
	{
		// Determine next state based on current action
		if (!CurrentTargetState.CurrentAction.IsEmpty() &&
			!CurrentTargetState.CurrentAction.Equals(TEXT("idle"), ESearchCase::IgnoreCase))
		{
			Agent->SetAgentState(EBBAgentState::SocialAction);
		}
		else
		{
			Agent->SetAgentState(EBBAgentState::Idle);
		}
	}

	UE_LOG(LogBigBrotherSim, Verbose, TEXT("AI: Move completed for %s (Result: %s)"),
		Agent ? *Agent->GetDisplayName() : TEXT("Unknown"),
		Result.IsSuccess() ? TEXT("Success") : TEXT("Failed"));
}

void ABBAgentAIController::InterpolateToTargetPosition(float DeltaTime)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	FVector CurrentPos = ControlledPawn->GetActorLocation();
	float DistSq = FVector::DistSquared(CurrentPos, TargetWorldPosition);

	if (DistSq < AcceptanceRadius * AcceptanceRadius)
	{
		bHasMoveCommand = false;
		ABBAgentCharacter* Agent = GetAgentCharacter();
		if (Agent && Agent->GetAgentState() == EBBAgentState::Walking)
		{
			Agent->SetAgentState(EBBAgentState::Idle);
		}
		return;
	}

	FVector NewPos = FMath::VInterpTo(CurrentPos, TargetWorldPosition, DeltaTime, PositionInterpSpeed);
	ControlledPawn->SetActorLocation(NewPos);

	// Face movement direction
	FVector Dir = (TargetWorldPosition - CurrentPos).GetSafeNormal2D();
	if (!Dir.IsNearlyZero())
	{
		FRotator DesiredRot = Dir.Rotation();
		FRotator CurrentRot = ControlledPawn->GetActorRotation();
		FRotator NewRot = FMath::RInterpTo(CurrentRot, DesiredRot, DeltaTime, PositionInterpSpeed * 2.f);
		ControlledPawn->SetActorRotation(NewRot);
	}
}

void ABBAgentAIController::HandleStateTransition(const FBBParticipantState& OldState, const FBBParticipantState& NewState)
{
	ABBAgentCharacter* Agent = GetAgentCharacter();
	if (!Agent)
	{
		return;
	}

	// Confessing state check
	if (NewState.CurrentAction.Equals(TEXT("confessing"), ESearchCase::IgnoreCase))
	{
		Agent->SetAgentState(EBBAgentState::Confessing);
		return;
	}

	// Mood change logging
	if (OldState.Mood != NewState.Mood)
	{
		UE_LOG(LogBigBrotherSim, Verbose, TEXT("AI: %s mood changed from %d to %d"),
			*Agent->GetDisplayName(), static_cast<int32>(OldState.Mood), static_cast<int32>(NewState.Mood));
	}

	// Head of Household status change
	if (!OldState.bIsHeadOfHousehold && NewState.bIsHeadOfHousehold)
	{
		UE_LOG(LogBigBrotherSim, Log, TEXT("AI: %s became Head of Household!"), *Agent->GetDisplayName());
	}

	// Nomination status change
	if (!OldState.bIsNominated && NewState.bIsNominated)
	{
		UE_LOG(LogBigBrotherSim, Log, TEXT("AI: %s has been nominated!"), *Agent->GetDisplayName());
	}
}
