// Copyright Big Brother AI Simulator. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BBSimDataSubsystem.h"
#include "BBReplayManager.generated.h"

UENUM(BlueprintType)
enum class EBBReplayState : uint8
{
	Inactive	UMETA(DisplayName = "Inactive"),
	Playing		UMETA(DisplayName = "Playing"),
	Paused		UMETA(DisplayName = "Paused")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReplayStateChanged, EBBReplayState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReplayTickChanged, int32, ReplayTick);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BIGBROTHERSIM_API UBBReplayManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UBBReplayManager();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Record a snapshot for a given tick */
	UFUNCTION(BlueprintCallable, Category = "BB|Replay")
	void RecordTick(int32 Tick, const FBBSnapshot& Snapshot);

	/** Start replay from a specific tick range */
	UFUNCTION(BlueprintCallable, Category = "BB|Replay")
	void StartReplay(int32 FromTick, int32 ToTick);

	/** Stop replay */
	UFUNCTION(BlueprintCallable, Category = "BB|Replay")
	void StopReplay();

	/** Pause or resume replay */
	UFUNCTION(BlueprintCallable, Category = "BB|Replay")
	void ToggleReplayPause();

	/** Scrub replay forward */
	UFUNCTION(BlueprintCallable, Category = "BB|Replay")
	void ScrubForward();

	/** Scrub replay backward */
	UFUNCTION(BlueprintCallable, Category = "BB|Replay")
	void ScrubBackward();

	/** Set replay speed multiplier */
	UFUNCTION(BlueprintCallable, Category = "BB|Replay")
	void SetReplaySpeed(float Speed);

	/** Get current replay state */
	UFUNCTION(BlueprintPure, Category = "BB|Replay")
	EBBReplayState GetReplayState() const { return ReplayState; }

	/** Get current replay tick */
	UFUNCTION(BlueprintPure, Category = "BB|Replay")
	int32 GetCurrentReplayTick() const { return CurrentReplayTick; }

	/** Get replay start tick */
	UFUNCTION(BlueprintPure, Category = "BB|Replay")
	int32 GetReplayStartTick() const { return ReplayStartTick; }

	/** Get replay end tick */
	UFUNCTION(BlueprintPure, Category = "BB|Replay")
	int32 GetReplayEndTick() const { return ReplayEndTick; }

	/** Get replay speed */
	UFUNCTION(BlueprintPure, Category = "BB|Replay")
	float GetReplaySpeed() const { return ReplaySpeedMultiplier; }

	/** Get snapshot at a replay tick (interpolated) */
	UFUNCTION(BlueprintCallable, Category = "BB|Replay")
	FBBSnapshot GetSnapshotAtReplayTick(int32 Tick) const;

	/** Check if a tick is recorded */
	UFUNCTION(BlueprintPure, Category = "BB|Replay")
	bool HasRecordedTick(int32 Tick) const;

	/** Get events associated with recorded ticks in range */
	UFUNCTION(BlueprintCallable, Category = "BB|Replay")
	TArray<FBBGameEvent> GetEventsInReplayRange() const;

	UPROPERTY(BlueprintAssignable, Category = "BB|Events")
	FOnReplayStateChanged OnReplayStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "BB|Events")
	FOnReplayTickChanged OnReplayTickChanged;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "BB|Replay")
	int32 MaxRecordedTicks = 500;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Replay")
	float BaseSecondsPerReplayTick = 1.0f;

private:
	void AdvanceReplayTick();
	FBBSnapshot InterpolateSnapshots(const FBBSnapshot& A, const FBBSnapshot& B, float Alpha) const;

	TMap<int32, FBBSnapshot> RecordedSnapshots;
	TArray<int32> RecordedTickOrder; // For eviction when buffer is full

	EBBReplayState ReplayState = EBBReplayState::Inactive;
	int32 CurrentReplayTick = 0;
	int32 ReplayStartTick = 0;
	int32 ReplayEndTick = 0;
	float ReplaySpeedMultiplier = 1.0f;
	float ReplayTickAccumulator = 0.f;
};
