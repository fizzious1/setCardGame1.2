// Copyright Big Brother AI Simulator. All Rights Reserved.

#include "BBDirectorCamera.h"
#include "BigBrotherSim.h"
#include "BBAgentCharacter.h"
#include "BBRoomVolume.h"
#include "BBHouseFloor.h"
#include "BBGameMode.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ABBDirectorCamera::ABBDirectorCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	RootComponent = CameraComp;
	CameraComp->FieldOfView = 75.f;

	TargetLocation = FVector(0.f, 0.f, 1500.f);
	TargetRotation = FRotator(-45.f, 0.f, 0.f);
}

void ABBDirectorCamera::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(TargetLocation);
	SetActorRotation(TargetRotation);
}

void ABBDirectorCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bManualOverride)
	{
		UpdateManualCamera(DeltaTime);
	}
	else if (bAutoDirecting)
	{
		UpdateAutoDirector(DeltaTime);
	}

	InterpolateCameraTransform(DeltaTime);

	// Clear old events
	RecentEventClearTimer += DeltaTime;
	if (RecentEventClearTimer > 10.0f)
	{
		RecentEvents.Empty();
		RecentEventClearTimer = 0.f;
	}
}

void ABBDirectorCamera::SetAutoDirecting(bool bEnable)
{
	bAutoDirecting = bEnable;
	if (bEnable)
	{
		bManualOverride = false;
	}
}

void ABBDirectorCamera::SetManualOverride(bool bEnable)
{
	bManualOverride = bEnable;
	if (bEnable)
	{
		bAutoDirecting = false;
	}
}

void ABBDirectorCamera::FocusOnRoom(ABBRoomVolume* Room)
{
	if (!Room)
	{
		return;
	}

	FocusedRoom = Room;
	FocusedAgent = nullptr;
	CurrentShotType = EBBShotType::Wide;

	FVector RoomPos = Room->GetActorLocation();
	FVector Ext = Room->GetRoomExtents();
	float RoomSize = FMath::Max(Ext.X, Ext.Y);

	TargetLocation = RoomPos + FVector(0.f, -RoomSize * 0.5f, RoomSize * 0.8f);
	TargetRotation = (RoomPos - TargetLocation).Rotation();

	TimeSinceLastCut = 0.f;
}

void ABBDirectorCamera::FocusOnAgent(ABBAgentCharacter* Agent)
{
	if (!Agent)
	{
		return;
	}

	FocusedAgent = Agent;
	CurrentShotType = EBBShotType::CloseUp;

	FVector AgentPos = Agent->GetActorLocation();
	FVector Forward = Agent->GetActorForwardVector();

	TargetLocation = AgentPos - Forward * CloseUpDistance + FVector(0.f, 0.f, 80.f);
	TargetRotation = (AgentPos + FVector(0.f, 0.f, 60.f) - TargetLocation).Rotation();

	TimeSinceLastCut = 0.f;
}

void ABBDirectorCamera::SwitchToOverview(const FVector& OverviewPos, const FRotator& OverviewRot)
{
	CurrentShotType = EBBShotType::Overview;
	TargetLocation = OverviewPos;
	TargetRotation = OverviewRot;
	FocusedRoom = nullptr;
	FocusedAgent = nullptr;
	TimeSinceLastCut = 0.f;
}

void ABBDirectorCamera::OnEventOccurred(const FBBGameEvent& Event)
{
	RecentEvents.Add(Event);
	RecentEventClearTimer = 0.f;

	// High drama events trigger immediate camera response
	if (Event.DramaScore > 0.7f)
	{
		ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
		if (!GM)
		{
			return;
		}

		// Zoom in on event participants
		if (Event.ParticipantIds.Num() > 0)
		{
			ABBAgentCharacter* PrimaryAgent = GM->GetAgentByParticipantId(Event.ParticipantIds[0]);
			if (PrimaryAgent)
			{
				float DramaDistance = CloseUpDistance * DramaZoomMultiplier;
				FVector AgentPos = PrimaryAgent->GetActorLocation();
				FVector Forward = PrimaryAgent->GetActorForwardVector();

				TargetLocation = AgentPos - Forward * DramaDistance + FVector(0.f, 0.f, 60.f);
				TargetRotation = (AgentPos + FVector(0.f, 0.f, 50.f) - TargetLocation).Rotation();
				CurrentShotType = EBBShotType::CloseUp;
				TimeSinceLastCut = 0.f;

				UE_LOG(LogBigBrotherSim, Log, TEXT("Director: Drama zoom on %s (score: %.2f)"),
					*PrimaryAgent->GetDisplayName(), Event.DramaScore);
			}
		}
	}
}

void ABBDirectorCamera::AddMovementInput(const FVector& Direction, float Scale)
{
	ManualMoveDelta += Direction * Scale;
}

void ABBDirectorCamera::AddLookInput(float Yaw, float Pitch)
{
	ManualYawDelta += Yaw;
	ManualPitchDelta += Pitch;
}

void ABBDirectorCamera::UpdateAutoDirector(float DeltaTime)
{
	TimeSinceLastCut += DeltaTime;

	// Update focus on tracked agent
	if (FocusedAgent.IsValid() && CurrentShotType == EBBShotType::CloseUp)
	{
		FVector AgentPos = FocusedAgent->GetActorLocation();
		FVector Forward = FocusedAgent->GetActorForwardVector();
		TargetLocation = AgentPos - Forward * CloseUpDistance + FVector(0.f, 0.f, 80.f);
		TargetRotation = (AgentPos + FVector(0.f, 0.f, 60.f) - TargetLocation).Rotation();
	}

	// Time for a new shot
	if (TimeSinceLastCut >= CutInterval)
	{
		ChooseNextShot();
	}
}

void ABBDirectorCamera::ChooseNextShot()
{
	TimeSinceLastCut = 0.f;

	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return;
	}

	ABBHouseFloor* House = GM->GetHouseFloor();
	if (!House)
	{
		return;
	}

	// Score rooms based on recent events and occupants
	TArray<ABBRoomVolume*> Rooms = House->GetAllRoomVolumes();
	ABBRoomVolume* BestRoom = nullptr;
	float BestScore = -1.f;

	for (ABBRoomVolume* Room : Rooms)
	{
		float Score = ScoreRoom(Room);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestRoom = Room;
		}
	}

	// Decide shot type
	float Roll = FMath::FRand();

	if (BestScore > 5.f && Roll < 0.4f)
	{
		// High drama room - pick an agent for close-up
		TArray<ABBAgentCharacter*> Agents = GM->GetAllAgents();
		for (ABBAgentCharacter* Agent : Agents)
		{
			if (BestRoom && Agent->GetCurrentRoomId() == BestRoom->GetRoomId())
			{
				FocusOnAgent(Agent);
				return;
			}
		}
	}

	if (Roll < 0.15f)
	{
		// Overview shot
		SwitchToOverview(House->GetOverviewCameraPosition(), House->GetOverviewCameraRotation());
	}
	else if (BestRoom)
	{
		// Focus on the most interesting room
		FocusOnRoom(BestRoom);
	}
}

float ABBDirectorCamera::ScoreRoom(ABBRoomVolume* Room) const
{
	if (!Room)
	{
		return 0.f;
	}

	float Score = 0.f;

	// Score based on recent events in this room
	for (const FBBGameEvent& Evt : RecentEvents)
	{
		if (Evt.RoomId == Room->GetRoomId())
		{
			Score += Evt.DramaScore * 10.f;

			// Bonus for high-drama event types
			switch (Evt.EventType)
			{
			case EBBEventType::Conflict: Score += 5.f; break;
			case EBBEventType::Betrayal: Score += 8.f; break;
			case EBBEventType::Nomination: Score += 6.f; break;
			case EBBEventType::Eviction: Score += 10.f; break;
			case EBBEventType::Romance: Score += 3.f; break;
			case EBBEventType::Alliance: Score += 2.f; break;
			default: break;
			}
		}
	}

	// Bonus for confessional room type (always interesting)
	if (Room->GetRoomType() == EBBRoomType::Confessional)
	{
		Score += 2.f;
	}

	// Avoid showing the same room repeatedly
	if (FocusedRoom.IsValid() && FocusedRoom.Get() == Room)
	{
		Score *= 0.3f;
	}

	// Small random factor for variety
	Score += FMath::FRand() * 2.f;

	return Score;
}

void ABBDirectorCamera::UpdateManualCamera(float DeltaTime)
{
	// Apply manual movement
	FRotator CurrentRot = GetActorRotation();
	FVector Forward = CurrentRot.Vector();
	FVector Right = FRotationMatrix(CurrentRot).GetScaledAxis(EAxis::Y);
	FVector Up = FVector::UpVector;

	FVector MoveDelta = (Forward * ManualMoveDelta.X + Right * ManualMoveDelta.Y + Up * ManualMoveDelta.Z)
		* ManualMoveSpeed * DeltaTime;

	TargetLocation = GetActorLocation() + MoveDelta;

	// Apply manual look
	TargetRotation = GetActorRotation();
	TargetRotation.Yaw += ManualYawDelta * ManualLookSpeed;
	TargetRotation.Pitch = FMath::Clamp(TargetRotation.Pitch + ManualPitchDelta * ManualLookSpeed, -89.f, 89.f);

	// Reset deltas
	ManualMoveDelta = FVector::ZeroVector;
	ManualYawDelta = 0.f;
	ManualPitchDelta = 0.f;
}

void ABBDirectorCamera::InterpolateCameraTransform(float DeltaTime)
{
	FVector CurrentLoc = GetActorLocation();
	FRotator CurrentRot = GetActorRotation();

	float Speed = bManualOverride ? 15.f : InterpSpeed;

	FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLocation, DeltaTime, Speed);
	FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRotation, DeltaTime, Speed);

	SetActorLocation(NewLoc);
	SetActorRotation(NewRot);
}
