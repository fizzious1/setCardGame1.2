// Copyright Big Brother AI Simulator. All Rights Reserved.

#include "BBAgentCharacter.h"
#include "BigBrotherSim.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BBAgentAIController.h"

ABBAgentCharacter::ABBAgentCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = ABBAgentAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Name tag widget above head
	NameTagWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("NameTagWidget"));
	NameTagWidget->SetupAttachment(GetMesh());
	NameTagWidget->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	NameTagWidget->SetWidgetSpace(EWidgetSpace::Screen);
	NameTagWidget->SetDrawSize(FVector2D(200.f, 40.f));

	// Mood indicator widget above name tag
	MoodIndicatorWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("MoodIndicatorWidget"));
	MoodIndicatorWidget->SetupAttachment(GetMesh());
	MoodIndicatorWidget->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	MoodIndicatorWidget->SetWidgetSpace(EWidgetSpace::Screen);
	MoodIndicatorWidget->SetDrawSize(FVector2D(50.f, 50.f));

	// Movement setup
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	}

	bUseControllerRotationYaw = false;
}

void ABBAgentCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ABBAgentCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bHasTargetPosition)
	{
		InterpolateToTargetPosition(DeltaTime);
	}
}

void ABBAgentCharacter::InitializeParticipant(const FBBParticipant& InParticipant)
{
	ParticipantData = InParticipant;
	CurrentState.ParticipantId = InParticipant.ParticipantId;

	UpdateNameTag();
	UpdateMoodIndicator();

	UE_LOG(LogBigBrotherSim, Log, TEXT("Agent initialized: %s (ID: %s)"),
		*ParticipantData.DisplayName, *ParticipantData.ParticipantId);
}

void ABBAgentCharacter::UpdateFromSnapshot(const FBBParticipantState& State)
{
	FString PreviousRoomId = CurrentState.CurrentRoomId;
	CurrentState = State;

	// Set target position for interpolation
	if (!State.WorldPosition.IsNearlyZero())
	{
		TargetPosition = State.WorldPosition;
		TargetRotation = State.WorldRotation;
		bHasTargetPosition = true;
	}

	// Determine agent state from action
	if (State.CurrentAction.Equals(TEXT("confessing"), ESearchCase::IgnoreCase))
	{
		SetAgentState(EBBAgentState::Confessing);
	}
	else if (PreviousRoomId != State.CurrentRoomId && !PreviousRoomId.IsEmpty())
	{
		SetAgentState(EBBAgentState::Walking);
	}
	else if (!State.CurrentAction.IsEmpty() &&
		!State.CurrentAction.Equals(TEXT("idle"), ESearchCase::IgnoreCase))
	{
		SetAgentState(EBBAgentState::SocialAction);
	}
	else
	{
		SetAgentState(EBBAgentState::Idle);
	}

	UpdateMoodIndicator();
}

void ABBAgentCharacter::MoveToLocation(const FVector& TargetLocation)
{
	TargetPosition = TargetLocation;
	bHasTargetPosition = true;
	SetAgentState(EBBAgentState::Walking);

	// Use AI controller for nav mesh pathfinding
	AAIController* AIC = Cast<AAIController>(GetController());
	if (AIC)
	{
		AIC->MoveToLocation(TargetLocation, 50.f, true, true, false, true);
	}
}

void ABBAgentCharacter::PlaySocialAnimation(const FString& AnimationHint)
{
	UAnimMontage** MontagePtr = SocialAnimations.Find(AnimationHint);
	if (MontagePtr && *MontagePtr)
	{
		UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
		if (AnimInst)
		{
			AnimInst->Montage_Play(*MontagePtr, 1.0f);
			SetAgentState(EBBAgentState::SocialAction);
			UE_LOG(LogBigBrotherSim, Verbose, TEXT("%s playing animation: %s"),
				*ParticipantData.DisplayName, *AnimationHint);
		}
	}
	else
	{
		UE_LOG(LogBigBrotherSim, Verbose, TEXT("No animation montage found for hint: %s"), *AnimationHint);
	}
}

void ABBAgentCharacter::SetAgentState(EBBAgentState NewState)
{
	if (AgentState == NewState)
	{
		return;
	}

	EBBAgentState OldState = AgentState;
	AgentState = NewState;

	// Handle state transitions
	switch (NewState)
	{
	case EBBAgentState::Idle:
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->StopMovementImmediately();
		}
		break;

	case EBBAgentState::Walking:
		// Movement handled by AI controller
		break;

	case EBBAgentState::SocialAction:
		// Animations triggered separately
		break;

	case EBBAgentState::Confessing:
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->StopMovementImmediately();
		}
		break;
	}

	UE_LOG(LogBigBrotherSim, Verbose, TEXT("%s state: %d -> %d"),
		*ParticipantData.DisplayName, static_cast<int32>(OldState), static_cast<int32>(NewState));
}

void ABBAgentCharacter::UpdateNameTag()
{
	// Widget component text is set via the bound UMG widget blueprint
	// The widget reads GetDisplayName() and GetCurrentMood() from the owning actor
}

void ABBAgentCharacter::UpdateMoodIndicator()
{
	// Mood indicator color is driven by the UMG widget which reads GetMoodColor()
	// and GetCurrentMood() from the owning character
}

void ABBAgentCharacter::InterpolateToTargetPosition(float DeltaTime)
{
	FVector CurrentLoc = GetActorLocation();
	float DistSq = FVector::DistSquared(CurrentLoc, TargetPosition);

	if (DistSq < 25.f) // Within 5 units
	{
		bHasTargetPosition = false;
		if (AgentState == EBBAgentState::Walking)
		{
			SetAgentState(EBBAgentState::Idle);
		}
		return;
	}

	FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetPosition, DeltaTime, PositionInterpSpeed);
	SetActorLocation(NewLoc);

	FRotator CurrentRot = GetActorRotation();
	FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRotation, DeltaTime, PositionInterpSpeed);
	SetActorRotation(NewRot);
}

FLinearColor ABBAgentCharacter::GetMoodColor() const
{
	switch (CurrentState.Mood)
	{
	case EBBMood::Happy:	return FLinearColor(0.2f, 0.9f, 0.3f, 1.f);
	case EBBMood::Sad:		return FLinearColor(0.2f, 0.3f, 0.9f, 1.f);
	case EBBMood::Angry:	return FLinearColor(0.9f, 0.1f, 0.1f, 1.f);
	case EBBMood::Anxious:	return FLinearColor(0.9f, 0.7f, 0.1f, 1.f);
	case EBBMood::Excited:	return FLinearColor(1.0f, 0.8f, 0.0f, 1.f);
	case EBBMood::Scheming:	return FLinearColor(0.6f, 0.1f, 0.8f, 1.f);
	case EBBMood::Betrayed:	return FLinearColor(0.5f, 0.0f, 0.5f, 1.f);
	default:				return FLinearColor(0.7f, 0.7f, 0.7f, 1.f);
	}
}
