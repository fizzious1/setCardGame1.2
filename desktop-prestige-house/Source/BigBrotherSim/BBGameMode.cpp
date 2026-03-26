// Copyright Big Brother AI Simulator. All Rights Reserved.

#include "BBGameMode.h"
#include "BigBrotherSim.h"
#include "BBAgentCharacter.h"
#include "BBHouseFloor.h"
#include "BBReplayManager.h"
#include "BBPlayerController.h"
#include "BBHUD.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ABBGameMode::ABBGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.016f; // ~60fps tick

	DefaultPawnClass = nullptr;
	PlayerControllerClass = ABBPlayerController::StaticClass();
	HUDClass = ABBHUD::StaticClass();

	ReplayManager = CreateDefaultSubobject<UBBReplayManager>(TEXT("ReplayManager"));
}

void ABBGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogBigBrotherSim, Log, TEXT("BBGameMode BeginPlay - Loading season data..."));

	LoadSeasonData();
	SpawnHouseFloor();
	SpawnAgents();

	// Apply initial snapshot
	UBBSimDataSubsystem* DataSub = GetGameInstance()->GetSubsystem<UBBSimDataSubsystem>();
	if (DataSub && DataSub->IsSeasonLoaded())
	{
		FBBSnapshot InitialSnap = DataSub->GetSnapshotAtTick(0);
		ApplySnapshotToAgents(InitialSnap);
	}
}

void ABBGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SimSpeed == EBBSimSpeed::Paused)
	{
		return;
	}

	float SpeedMult = GetSpeedMultiplier();
	TickAccumulator += DeltaTime * SpeedMult;

	if (TickAccumulator >= SecondsPerTick)
	{
		TickAccumulator -= SecondsPerTick;
		AdvanceTick();
	}
}

void ABBGameMode::LoadSeasonData()
{
	UBBSimDataSubsystem* DataSub = GetGameInstance()->GetSubsystem<UBBSimDataSubsystem>();
	if (!DataSub)
	{
		UE_LOG(LogBigBrotherSim, Error, TEXT("BBSimDataSubsystem not found!"));
		return;
	}

	FString FullPath = FPaths::ProjectContentDir() / SeasonDataPath;
	if (DataSub->LoadSeasonFromFile(FullPath))
	{
		UE_LOG(LogBigBrotherSim, Log, TEXT("Season data loaded successfully from %s"), *FullPath);
	}
	else
	{
		UE_LOG(LogBigBrotherSim, Warning, TEXT("Failed to load season data from %s. Simulation will use empty data."), *FullPath);
	}
}

void ABBGameMode::SpawnHouseFloor()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (HouseFloorClass)
	{
		HouseFloor = World->SpawnActor<ABBHouseFloor>(HouseFloorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}
	else
	{
		HouseFloor = World->SpawnActor<ABBHouseFloor>(ABBHouseFloor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	}

	if (HouseFloor)
	{
		UBBSimDataSubsystem* DataSub = GetGameInstance()->GetSubsystem<UBBSimDataSubsystem>();
		if (DataSub && DataSub->IsSeasonLoaded())
		{
			HouseFloor->InitializeFromRoomData(DataSub->GetAllRooms());
		}
		UE_LOG(LogBigBrotherSim, Log, TEXT("House floor spawned."));
	}
}

void ABBGameMode::SpawnAgents()
{
	UBBSimDataSubsystem* DataSub = GetGameInstance()->GetSubsystem<UBBSimDataSubsystem>();
	if (!DataSub || !DataSub->IsSeasonLoaded())
	{
		UE_LOG(LogBigBrotherSim, Warning, TEXT("Cannot spawn agents - no season data loaded."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FBBSeason& Season = DataSub->GetSeason();
	for (const FBBParticipant& Participant : Season.Participants)
	{
		if (Participant.bEvicted)
		{
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ABBAgentCharacter* Agent = nullptr;
		FVector SpawnLoc = FVector(FMath::RandRange(-500.f, 500.f), FMath::RandRange(-500.f, 500.f), 0.f);

		if (AgentCharacterClass)
		{
			Agent = World->SpawnActor<ABBAgentCharacter>(AgentCharacterClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
		}
		else
		{
			Agent = World->SpawnActor<ABBAgentCharacter>(ABBAgentCharacter::StaticClass(), SpawnLoc, FRotator::ZeroRotator, SpawnParams);
		}

		if (Agent)
		{
			Agent->InitializeParticipant(Participant);
			SpawnedAgents.Add(Agent);
			UE_LOG(LogBigBrotherSim, Log, TEXT("Spawned agent: %s"), *Participant.DisplayName);
		}
	}

	UE_LOG(LogBigBrotherSim, Log, TEXT("Spawned %d agents."), SpawnedAgents.Num());
}

void ABBGameMode::TogglePlayPause()
{
	if (SimSpeed == EBBSimSpeed::Paused)
	{
		SimSpeed = EBBSimSpeed::Normal;
	}
	else
	{
		SimSpeed = EBBSimSpeed::Paused;
	}
	OnSimSpeedChanged.Broadcast(SimSpeed);
	UE_LOG(LogBigBrotherSim, Log, TEXT("Simulation %s"), SimSpeed == EBBSimSpeed::Paused ? TEXT("PAUSED") : TEXT("PLAYING"));
}

void ABBGameMode::SetSimSpeed(EBBSimSpeed NewSpeed)
{
	SimSpeed = NewSpeed;
	OnSimSpeedChanged.Broadcast(SimSpeed);
}

void ABBGameMode::StepForward()
{
	SimSpeed = EBBSimSpeed::Paused;
	AdvanceTick();
	OnSimSpeedChanged.Broadcast(SimSpeed);
}

void ABBGameMode::StepBackward()
{
	SimSpeed = EBBSimSpeed::Paused;
	if (CurrentTick > 0)
	{
		CurrentTick--;
		UBBSimDataSubsystem* DataSub = GetGameInstance()->GetSubsystem<UBBSimDataSubsystem>();
		if (DataSub)
		{
			FBBSnapshot Snap = DataSub->ScrubToTick(CurrentTick);
			ApplySnapshotToAgents(Snap);
			ProcessEventsAtTick(CurrentTick);
		}
		OnTickAdvanced.Broadcast(CurrentTick);
	}
	OnSimSpeedChanged.Broadcast(SimSpeed);
}

void ABBGameMode::ScrubToTick(int32 Tick)
{
	UBBSimDataSubsystem* DataSub = GetGameInstance()->GetSubsystem<UBBSimDataSubsystem>();
	if (!DataSub)
	{
		return;
	}

	CurrentTick = FMath::Clamp(Tick, 0, DataSub->GetTotalTicks());
	FBBSnapshot Snap = DataSub->ScrubToTick(CurrentTick);
	ApplySnapshotToAgents(Snap);
	ProcessEventsAtTick(CurrentTick);
	OnTickAdvanced.Broadcast(CurrentTick);
}

void ABBGameMode::AdvanceTick()
{
	UBBSimDataSubsystem* DataSub = GetGameInstance()->GetSubsystem<UBBSimDataSubsystem>();
	if (!DataSub || !DataSub->IsSeasonLoaded())
	{
		return;
	}

	if (CurrentTick >= DataSub->GetTotalTicks())
	{
		SimSpeed = EBBSimSpeed::Paused;
		OnSimSpeedChanged.Broadcast(SimSpeed);
		UE_LOG(LogBigBrotherSim, Log, TEXT("Reached end of simulation."));
		return;
	}

	CurrentTick++;

	FBBSnapshot Snap = DataSub->ScrubToTick(CurrentTick);
	ApplySnapshotToAgents(Snap);
	ProcessEventsAtTick(CurrentTick);

	// Feed to replay manager
	if (ReplayManager)
	{
		ReplayManager->RecordTick(CurrentTick, Snap);
	}

	OnTickAdvanced.Broadcast(CurrentTick);
}

void ABBGameMode::ApplySnapshotToAgents(const FBBSnapshot& Snapshot)
{
	for (const FBBParticipantState& State : Snapshot.ParticipantStates)
	{
		ABBAgentCharacter* Agent = GetAgentByParticipantId(State.ParticipantId);
		if (Agent)
		{
			Agent->UpdateFromSnapshot(State);
		}
	}
}

void ABBGameMode::ProcessEventsAtTick(int32 Tick)
{
	UBBSimDataSubsystem* DataSub = GetGameInstance()->GetSubsystem<UBBSimDataSubsystem>();
	if (!DataSub)
	{
		return;
	}

	TArray<FBBGameEvent> Events = DataSub->GetEventsAtTick(Tick);
	for (const FBBGameEvent& Event : Events)
	{
		OnEventTriggered.Broadcast(Event);
		UE_LOG(LogBigBrotherSim, Verbose, TEXT("Event at tick %d: %s - %s"), Tick,
			*Event.EventId, *Event.Description);
	}
}

ABBAgentCharacter* ABBGameMode::GetAgentByParticipantId(const FString& ParticipantId) const
{
	for (ABBAgentCharacter* Agent : SpawnedAgents)
	{
		if (Agent && Agent->GetParticipantId() == ParticipantId)
		{
			return Agent;
		}
	}
	return nullptr;
}

float ABBGameMode::GetSpeedMultiplier() const
{
	switch (SimSpeed)
	{
	case EBBSimSpeed::Normal: return 1.0f;
	case EBBSimSpeed::Fast: return 2.0f;
	case EBBSimSpeed::VeryFast: return 4.0f;
	default: return 0.0f;
	}
}
