// Copyright Big Brother AI Simulator. All Rights Reserved.

#include "BBReplayManager.h"
#include "BigBrotherSim.h"
#include "BBGameMode.h"
#include "Kismet/GameplayStatics.h"

UBBReplayManager::UBBReplayManager()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBBReplayManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ReplayState != EBBReplayState::Playing)
	{
		return;
	}

	float EffectiveSeconds = BaseSecondsPerReplayTick / FMath::Max(ReplaySpeedMultiplier, 0.1f);
	ReplayTickAccumulator += DeltaTime;

	if (ReplayTickAccumulator >= EffectiveSeconds)
	{
		ReplayTickAccumulator -= EffectiveSeconds;
		AdvanceReplayTick();
	}
}

void UBBReplayManager::RecordTick(int32 Tick, const FBBSnapshot& Snapshot)
{
	// Evict oldest if at capacity
	if (RecordedSnapshots.Num() >= MaxRecordedTicks && !RecordedSnapshots.Contains(Tick))
	{
		if (RecordedTickOrder.Num() > 0)
		{
			int32 OldestTick = RecordedTickOrder[0];
			RecordedSnapshots.Remove(OldestTick);
			RecordedTickOrder.RemoveAt(0);
		}
	}

	RecordedSnapshots.Add(Tick, Snapshot);

	if (!RecordedTickOrder.Contains(Tick))
	{
		RecordedTickOrder.Add(Tick);
	}
}

void UBBReplayManager::StartReplay(int32 FromTick, int32 ToTick)
{
	ReplayStartTick = FromTick;
	ReplayEndTick = ToTick;
	CurrentReplayTick = FromTick;
	ReplayTickAccumulator = 0.f;
	ReplayState = EBBReplayState::Playing;

	OnReplayStateChanged.Broadcast(ReplayState);
	OnReplayTickChanged.Broadcast(CurrentReplayTick);

	// Apply the snapshot at the start tick
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		GM->ScrubToTick(CurrentReplayTick);
	}

	UE_LOG(LogBigBrotherSim, Log, TEXT("Replay started: ticks %d to %d"), FromTick, ToTick);
}

void UBBReplayManager::StopReplay()
{
	ReplayState = EBBReplayState::Inactive;
	ReplayTickAccumulator = 0.f;
	OnReplayStateChanged.Broadcast(ReplayState);
	UE_LOG(LogBigBrotherSim, Log, TEXT("Replay stopped."));
}

void UBBReplayManager::ToggleReplayPause()
{
	if (ReplayState == EBBReplayState::Playing)
	{
		ReplayState = EBBReplayState::Paused;
	}
	else if (ReplayState == EBBReplayState::Paused)
	{
		ReplayState = EBBReplayState::Playing;
	}
	OnReplayStateChanged.Broadcast(ReplayState);
}

void UBBReplayManager::ScrubForward()
{
	if (CurrentReplayTick < ReplayEndTick)
	{
		CurrentReplayTick++;

		ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
		if (GM)
		{
			GM->ScrubToTick(CurrentReplayTick);
		}
		OnReplayTickChanged.Broadcast(CurrentReplayTick);
	}
}

void UBBReplayManager::ScrubBackward()
{
	if (CurrentReplayTick > ReplayStartTick)
	{
		CurrentReplayTick--;

		ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
		if (GM)
		{
			GM->ScrubToTick(CurrentReplayTick);
		}
		OnReplayTickChanged.Broadcast(CurrentReplayTick);
	}
}

void UBBReplayManager::SetReplaySpeed(float Speed)
{
	ReplaySpeedMultiplier = FMath::Clamp(Speed, 0.25f, 8.0f);
	UE_LOG(LogBigBrotherSim, Log, TEXT("Replay speed set to %.1fx"), ReplaySpeedMultiplier);
}

FBBSnapshot UBBReplayManager::GetSnapshotAtReplayTick(int32 Tick) const
{
	// Exact match
	if (const FBBSnapshot* Found = RecordedSnapshots.Find(Tick))
	{
		return *Found;
	}

	// Interpolate between nearest recorded ticks
	int32 BeforeTick = -1;
	int32 AfterTick = -1;

	for (int32 RecTick : RecordedTickOrder)
	{
		if (RecTick <= Tick)
		{
			BeforeTick = RecTick;
		}
		if (RecTick >= Tick && AfterTick < 0)
		{
			AfterTick = RecTick;
			break;
		}
	}

	if (BeforeTick >= 0 && AfterTick >= 0 && BeforeTick != AfterTick)
	{
		float Alpha = static_cast<float>(Tick - BeforeTick) / static_cast<float>(AfterTick - BeforeTick);
		return InterpolateSnapshots(RecordedSnapshots[BeforeTick], RecordedSnapshots[AfterTick], Alpha);
	}

	if (BeforeTick >= 0)
	{
		return RecordedSnapshots[BeforeTick];
	}

	if (AfterTick >= 0)
	{
		return RecordedSnapshots[AfterTick];
	}

	return FBBSnapshot();
}

bool UBBReplayManager::HasRecordedTick(int32 Tick) const
{
	return RecordedSnapshots.Contains(Tick);
}

TArray<FBBGameEvent> UBBReplayManager::GetEventsInReplayRange() const
{
	UBBSimDataSubsystem* DataSub = nullptr;
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		UGameInstance* GI = GM->GetGameInstance();
		if (GI)
		{
			DataSub = GI->GetSubsystem<UBBSimDataSubsystem>();
		}
	}

	if (DataSub)
	{
		return DataSub->GetEventsInRange(ReplayStartTick, ReplayEndTick);
	}

	return TArray<FBBGameEvent>();
}

void UBBReplayManager::AdvanceReplayTick()
{
	if (CurrentReplayTick >= ReplayEndTick)
	{
		ReplayState = EBBReplayState::Paused;
		OnReplayStateChanged.Broadcast(ReplayState);
		UE_LOG(LogBigBrotherSim, Log, TEXT("Replay reached end at tick %d"), CurrentReplayTick);
		return;
	}

	CurrentReplayTick++;

	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		GM->ScrubToTick(CurrentReplayTick);
	}

	OnReplayTickChanged.Broadcast(CurrentReplayTick);
}

FBBSnapshot UBBReplayManager::InterpolateSnapshots(const FBBSnapshot& A, const FBBSnapshot& B, float Alpha) const
{
	FBBSnapshot Result;
	Result.Tick = FMath::RoundToInt32(FMath::Lerp(static_cast<float>(A.Tick), static_cast<float>(B.Tick), Alpha));
	Result.Day = A.Day;
	Result.Hour = A.Hour;
	Result.ActiveEvents = Alpha < 0.5f ? A.ActiveEvents : B.ActiveEvents;

	for (int32 i = 0; i < A.ParticipantStates.Num(); ++i)
	{
		FBBParticipantState InterpState = A.ParticipantStates[i];

		for (const FBBParticipantState& BState : B.ParticipantStates)
		{
			if (BState.ParticipantId == InterpState.ParticipantId)
			{
				InterpState.WorldPosition = FMath::Lerp(InterpState.WorldPosition, BState.WorldPosition, Alpha);
				InterpState.WorldRotation = FMath::Lerp(InterpState.WorldRotation, BState.WorldRotation, Alpha);
				InterpState.MoodIntensity = FMath::Lerp(InterpState.MoodIntensity, BState.MoodIntensity, Alpha);
				InterpState.StressLevel = FMath::Lerp(InterpState.StressLevel, BState.StressLevel, Alpha);

				if (Alpha > 0.5f)
				{
					InterpState.Mood = BState.Mood;
					InterpState.CurrentRoomId = BState.CurrentRoomId;
					InterpState.CurrentAction = BState.CurrentAction;
				}
				break;
			}
		}

		Result.ParticipantStates.Add(InterpState);
	}

	return Result;
}
