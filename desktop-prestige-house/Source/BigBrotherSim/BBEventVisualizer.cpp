// Copyright Big Brother AI Simulator. All Rights Reserved.

#include "BBEventVisualizer.h"
#include "BigBrotherSim.h"
#include "BBAgentCharacter.h"
#include "BBGameMode.h"
#include "BBDirectorCamera.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ABBEventVisualizer::ABBEventVisualizer()
{
	PrimaryActorTick.bCanEverTick = true;

	// Initialize default event configs
	{
		FBBEventVisualizationConfig AllianceCfg;
		AllianceCfg.AnimationHint = TEXT("Handshake");
		AllianceCfg.PopupText = TEXT("ALLIANCE FORMED");
		AllianceCfg.PopupColor = FLinearColor(0.1f, 0.9f, 0.2f, 1.f);
		AllianceCfg.bCameraCloseUp = true;
		EventConfigs.Add(EBBEventType::Alliance, AllianceCfg);
	}
	{
		FBBEventVisualizationConfig ConflictCfg;
		ConflictCfg.AnimationHint = TEXT("Argument");
		ConflictCfg.PopupText = TEXT("CONFLICT");
		ConflictCfg.PopupColor = FLinearColor(0.9f, 0.1f, 0.1f, 1.f);
		ConflictCfg.bCameraCloseUp = true;
		ConflictCfg.bScreenShake = true;
		ConflictCfg.ScreenShakeIntensity = 1.5f;
		EventConfigs.Add(EBBEventType::Conflict, ConflictCfg);
	}
	{
		FBBEventVisualizationConfig BetrayalCfg;
		BetrayalCfg.AnimationHint = TEXT("Betray");
		BetrayalCfg.PopupText = TEXT("BETRAYAL!");
		BetrayalCfg.PopupColor = FLinearColor(0.6f, 0.0f, 0.8f, 1.f);
		BetrayalCfg.bCameraCloseUp = true;
		BetrayalCfg.bSlowMotion = true;
		BetrayalCfg.SlowMotionDuration = 2.5f;
		BetrayalCfg.SlowMotionTimeDilation = 0.25f;
		EventConfigs.Add(EBBEventType::Betrayal, BetrayalCfg);
	}
	{
		FBBEventVisualizationConfig NominationCfg;
		NominationCfg.AnimationHint = TEXT("PointAt");
		NominationCfg.PopupText = TEXT("NOMINATED");
		NominationCfg.PopupColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.f);
		NominationCfg.bCameraCloseUp = true;
		EventConfigs.Add(EBBEventType::Nomination, NominationCfg);
	}
	{
		FBBEventVisualizationConfig EvictionCfg;
		EvictionCfg.AnimationHint = TEXT("Wave");
		EvictionCfg.PopupText = TEXT("EVICTED");
		EvictionCfg.PopupColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.f);
		EvictionCfg.bCameraCloseUp = true;
		EvictionCfg.bSlowMotion = true;
		EvictionCfg.SlowMotionDuration = 3.0f;
		EvictionCfg.SlowMotionTimeDilation = 0.4f;
		EventConfigs.Add(EBBEventType::Eviction, EvictionCfg);
	}
}

void ABBEventVisualizer::BeginPlay()
{
	Super::BeginPlay();

	// Register for game mode events
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		GM->OnEventTriggered.AddDynamic(this, &ABBEventVisualizer::OnEventTriggered);
	}
}

void ABBEventVisualizer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle slow motion timer
	if (bSlowMotionActive)
	{
		SlowMotionTimer += DeltaTime; // Real-time delta
		if (SlowMotionTimer >= SlowMotionDuration)
		{
			EndSlowMotion();
		}
	}

	// Clean up finished particle effects
	for (int32 i = ActiveParticles.Num() - 1; i >= 0; --i)
	{
		if (!ActiveParticles[i] || !ActiveParticles[i]->IsActive())
		{
			if (ActiveParticles[i])
			{
				ActiveParticles[i]->DestroyComponent();
			}
			ActiveParticles.RemoveAt(i);
		}
	}
}

void ABBEventVisualizer::VisualizeEvent(const FBBGameEvent& Event)
{
	FBBEventVisualizationConfig Config = GetConfigForEventType(Event.EventType);

	switch (Event.EventType)
	{
	case EBBEventType::Alliance:
		VisualizeAlliance(Event, Config);
		break;
	case EBBEventType::Conflict:
		VisualizeConflict(Event, Config);
		break;
	case EBBEventType::Betrayal:
		VisualizeBetrayal(Event, Config);
		break;
	case EBBEventType::Nomination:
		VisualizeNomination(Event, Config);
		break;
	case EBBEventType::Eviction:
		VisualizeEviction(Event, Config);
		break;
	default:
		VisualizeGenericEvent(Event, Config);
		break;
	}

	// Show popup
	if (!Config.PopupText.IsEmpty())
	{
		FString DisplayText = Event.Description.IsEmpty() ? Config.PopupText : Config.PopupText + TEXT(": ") + Event.Description;
		OnEventPopup.Broadcast(DisplayText, Config.PopupColor);
	}

	// Camera response
	TriggerCameraResponse(Event, Config);
}

void ABBEventVisualizer::OnEventTriggered(const FBBGameEvent& Event)
{
	VisualizeEvent(Event);
}

FBBEventVisualizationConfig ABBEventVisualizer::GetConfigForEventType(EBBEventType EventType) const
{
	if (const FBBEventVisualizationConfig* Found = EventConfigs.Find(EventType))
	{
		return *Found;
	}
	return FBBEventVisualizationConfig();
}

void ABBEventVisualizer::VisualizeAlliance(const FBBGameEvent& Event, const FBBEventVisualizationConfig& Config)
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;

	// Face participants toward each other and play handshake
	if (Event.ParticipantIds.Num() >= 2)
	{
		ABBAgentCharacter* AgentA = GM->GetAgentByParticipantId(Event.ParticipantIds[0]);
		ABBAgentCharacter* AgentB = GM->GetAgentByParticipantId(Event.ParticipantIds[1]);

		if (AgentA && AgentB)
		{
			FaceAgentsTowardEachOther(AgentA, AgentB);
			AgentA->PlaySocialAnimation(Config.AnimationHint);
			AgentB->PlaySocialAnimation(Config.AnimationHint);

			// Spawn green particle effect between them
			FVector MidPoint = (AgentA->GetActorLocation() + AgentB->GetActorLocation()) * 0.5f;
			if (Config.ParticleSystem)
			{
				SpawnParticleEffect(Config.ParticleSystem, MidPoint, Config.PopupColor);
			}
		}
	}

	if (Config.SoundCue)
	{
		ABBAgentCharacter* Primary = GM->GetAgentByParticipantId(Event.ParticipantIds[0]);
		if (Primary)
		{
			PlayEventSound(Config.SoundCue, Primary->GetActorLocation());
		}
	}

	UE_LOG(LogBigBrotherSim, Log, TEXT("Visualized ALLIANCE: %s"), *Event.Description);
}

void ABBEventVisualizer::VisualizeConflict(const FBBGameEvent& Event, const FBBEventVisualizationConfig& Config)
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;

	if (Event.ParticipantIds.Num() >= 2)
	{
		ABBAgentCharacter* AgentA = GM->GetAgentByParticipantId(Event.ParticipantIds[0]);
		ABBAgentCharacter* AgentB = GM->GetAgentByParticipantId(Event.ParticipantIds[1]);

		if (AgentA && AgentB)
		{
			FaceAgentsTowardEachOther(AgentA, AgentB);
			AgentA->PlaySocialAnimation(Config.AnimationHint);
			AgentB->PlaySocialAnimation(Config.AnimationHint);

			// Red particle effect
			FVector MidPoint = (AgentA->GetActorLocation() + AgentB->GetActorLocation()) * 0.5f;
			if (Config.ParticleSystem)
			{
				SpawnParticleEffect(Config.ParticleSystem, MidPoint, Config.PopupColor);
			}
		}
	}

	// Screen shake
	if (Config.bScreenShake)
	{
		TriggerScreenShake(Config.ScreenShakeIntensity);
	}

	if (Config.SoundCue)
	{
		ABBAgentCharacter* Primary = Event.ParticipantIds.Num() > 0 ? GM->GetAgentByParticipantId(Event.ParticipantIds[0]) : nullptr;
		if (Primary)
		{
			PlayEventSound(Config.SoundCue, Primary->GetActorLocation());
		}
	}

	UE_LOG(LogBigBrotherSim, Log, TEXT("Visualized CONFLICT: %s"), *Event.Description);
}

void ABBEventVisualizer::VisualizeBetrayal(const FBBGameEvent& Event, const FBBEventVisualizationConfig& Config)
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;

	// Slow motion for betrayal
	if (Config.bSlowMotion)
	{
		BeginSlowMotion(Config.SlowMotionDuration, Config.SlowMotionTimeDilation);
	}

	if (Event.ParticipantIds.Num() >= 1)
	{
		ABBAgentCharacter* Betrayer = GM->GetAgentByParticipantId(Event.ParticipantIds[0]);
		if (Betrayer)
		{
			Betrayer->PlaySocialAnimation(Config.AnimationHint);

			// Purple particle effect on betrayer
			if (Config.ParticleSystem)
			{
				SpawnParticleEffect(Config.ParticleSystem, Betrayer->GetActorLocation(), Config.PopupColor);
			}
		}
	}

	if (Config.SoundCue)
	{
		ABBAgentCharacter* Primary = Event.ParticipantIds.Num() > 0 ? GM->GetAgentByParticipantId(Event.ParticipantIds[0]) : nullptr;
		if (Primary)
		{
			PlayEventSound(Config.SoundCue, Primary->GetActorLocation());
		}
	}

	UE_LOG(LogBigBrotherSim, Log, TEXT("Visualized BETRAYAL: %s"), *Event.Description);
}

void ABBEventVisualizer::VisualizeNomination(const FBBGameEvent& Event, const FBBEventVisualizationConfig& Config)
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;

	// Spotlight on nominated participant
	for (const FString& Pid : Event.ParticipantIds)
	{
		ABBAgentCharacter* Agent = GM->GetAgentByParticipantId(Pid);
		if (Agent)
		{
			Agent->PlaySocialAnimation(Config.AnimationHint);

			if (Config.ParticleSystem)
			{
				SpawnParticleEffect(Config.ParticleSystem, Agent->GetActorLocation() + FVector(0.f, 0.f, 50.f), Config.PopupColor);
			}
		}
	}

	if (Config.SoundCue && Event.ParticipantIds.Num() > 0)
	{
		ABBAgentCharacter* Primary = GM->GetAgentByParticipantId(Event.ParticipantIds[0]);
		if (Primary)
		{
			PlayEventSound(Config.SoundCue, Primary->GetActorLocation());
		}
	}

	UE_LOG(LogBigBrotherSim, Log, TEXT("Visualized NOMINATION: %s"), *Event.Description);
}

void ABBEventVisualizer::VisualizeEviction(const FBBGameEvent& Event, const FBBEventVisualizationConfig& Config)
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;

	if (Config.bSlowMotion)
	{
		BeginSlowMotion(Config.SlowMotionDuration, Config.SlowMotionTimeDilation);
	}

	if (Event.ParticipantIds.Num() >= 1)
	{
		ABBAgentCharacter* Evicted = GM->GetAgentByParticipantId(Event.ParticipantIds[0]);
		if (Evicted)
		{
			// Wave farewell animation
			Evicted->PlaySocialAnimation(Config.AnimationHint);

			// Move toward "exit" - a point far from center
			FVector ExitPoint = Evicted->GetActorLocation() + Evicted->GetActorForwardVector() * 2000.f;
			Evicted->MoveToLocation(ExitPoint);

			if (Config.ParticleSystem)
			{
				SpawnParticleEffect(Config.ParticleSystem, Evicted->GetActorLocation(), Config.PopupColor);
			}
		}
	}

	if (Config.SoundCue && Event.ParticipantIds.Num() > 0)
	{
		ABBAgentCharacter* Primary = GM->GetAgentByParticipantId(Event.ParticipantIds[0]);
		if (Primary)
		{
			PlayEventSound(Config.SoundCue, Primary->GetActorLocation());
		}
	}

	UE_LOG(LogBigBrotherSim, Log, TEXT("Visualized EVICTION: %s"), *Event.Description);
}

void ABBEventVisualizer::VisualizeGenericEvent(const FBBGameEvent& Event, const FBBEventVisualizationConfig& Config)
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;

	// Play animation on first participant
	if (Event.ParticipantIds.Num() > 0 && !Config.AnimationHint.IsEmpty())
	{
		ABBAgentCharacter* Agent = GM->GetAgentByParticipantId(Event.ParticipantIds[0]);
		if (Agent)
		{
			Agent->PlaySocialAnimation(Config.AnimationHint);
		}
	}

	UE_LOG(LogBigBrotherSim, Verbose, TEXT("Visualized event: %s - %s"), *Event.EventId, *Event.Description);
}

void ABBEventVisualizer::FaceAgentsTowardEachOther(ABBAgentCharacter* A, ABBAgentCharacter* B)
{
	if (!A || !B) return;

	FVector APos = A->GetActorLocation();
	FVector BPos = B->GetActorLocation();

	FRotator AtoB = (BPos - APos).Rotation();
	FRotator BtoA = (APos - BPos).Rotation();

	A->SetActorRotation(FRotator(0.f, AtoB.Yaw, 0.f));
	B->SetActorRotation(FRotator(0.f, BtoA.Yaw, 0.f));
}

void ABBEventVisualizer::TriggerCameraResponse(const FBBGameEvent& Event, const FBBEventVisualizationConfig& Config)
{
	if (!Config.bCameraCloseUp)
	{
		return;
	}

	// Find director camera and have it focus on the event
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;

	TArray<AActor*> FoundCameras;
	UGameplayStatics::GetAllActorsOfClass(this, ABBDirectorCamera::StaticClass(), FoundCameras);

	if (FoundCameras.Num() > 0)
	{
		ABBDirectorCamera* Director = Cast<ABBDirectorCamera>(FoundCameras[0]);
		if (Director)
		{
			Director->OnEventOccurred(Event);
		}
	}
}

void ABBEventVisualizer::TriggerScreenShake(float Intensity)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC && DramaShake)
	{
		PC->ClientStartCameraShake(DramaShake, Intensity);
	}
}

void ABBEventVisualizer::BeginSlowMotion(float Duration, float TimeDilation)
{
	bSlowMotionActive = true;
	SlowMotionTimer = 0.f;
	SlowMotionDuration = Duration;

	UGameplayStatics::SetGlobalTimeDilation(this, TimeDilation);

	UE_LOG(LogBigBrotherSim, Log, TEXT("Slow motion started: %.1fs at %.2fx"), Duration, TimeDilation);
}

void ABBEventVisualizer::EndSlowMotion()
{
	bSlowMotionActive = false;
	SlowMotionTimer = 0.f;

	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);

	UE_LOG(LogBigBrotherSim, Log, TEXT("Slow motion ended."));
}

void ABBEventVisualizer::SpawnParticleEffect(UNiagaraSystem* System, const FVector& Location, const FLinearColor& Color)
{
	if (!System) return;

	UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this, System, Location, FRotator::ZeroRotator, FVector::OneVector, true, true);

	if (NC)
	{
		NC->SetColorParameter(FName(TEXT("ParticleColor")), Color);
		ActiveParticles.Add(NC);
	}
}

void ABBEventVisualizer::PlayEventSound(USoundBase* Sound, const FVector& Location)
{
	if (!Sound) return;
	UGameplayStatics::PlaySoundAtLocation(this, Sound, Location, 1.0f, 1.0f, 0.f);
}
