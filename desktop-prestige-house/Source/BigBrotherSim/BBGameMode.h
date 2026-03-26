// Copyright Big Brother AI Simulator. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BBSimDataSubsystem.h"
#include "BBGameMode.generated.h"

class ABBAgentCharacter;
class ABBHouseFloor;
class UBBReplayManager;

UENUM(BlueprintType)
enum class EBBSimSpeed : uint8
{
	Paused		UMETA(DisplayName = "Paused"),
	Normal		UMETA(DisplayName = "1x"),
	Fast		UMETA(DisplayName = "2x"),
	VeryFast	UMETA(DisplayName = "4x")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTickAdvanced, int32, NewTick);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSimSpeedChanged, EBBSimSpeed, NewSpeed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEventTriggered, const FBBGameEvent&, Event);

UCLASS()
class BIGBROTHERSIM_API ABBGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABBGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Play / Pause simulation */
	UFUNCTION(BlueprintCallable, Category = "BB|Simulation")
	void TogglePlayPause();

	UFUNCTION(BlueprintCallable, Category = "BB|Simulation")
	void SetSimSpeed(EBBSimSpeed NewSpeed);

	UFUNCTION(BlueprintCallable, Category = "BB|Simulation")
	void StepForward();

	UFUNCTION(BlueprintCallable, Category = "BB|Simulation")
	void StepBackward();

	UFUNCTION(BlueprintCallable, Category = "BB|Simulation")
	void ScrubToTick(int32 Tick);

	UFUNCTION(BlueprintPure, Category = "BB|Simulation")
	int32 GetCurrentTick() const { return CurrentTick; }

	UFUNCTION(BlueprintPure, Category = "BB|Simulation")
	bool IsPlaying() const { return SimSpeed != EBBSimSpeed::Paused; }

	UFUNCTION(BlueprintPure, Category = "BB|Simulation")
	EBBSimSpeed GetSimSpeed() const { return SimSpeed; }

	UFUNCTION(BlueprintPure, Category = "BB|Simulation")
	ABBHouseFloor* GetHouseFloor() const { return HouseFloor; }

	UFUNCTION(BlueprintCallable, Category = "BB|Simulation")
	ABBAgentCharacter* GetAgentByParticipantId(const FString& ParticipantId) const;

	UFUNCTION(BlueprintCallable, Category = "BB|Simulation")
	TArray<ABBAgentCharacter*> GetAllAgents() const { return SpawnedAgents; }

	UPROPERTY(BlueprintAssignable, Category = "BB|Events")
	FOnTickAdvanced OnTickAdvanced;

	UPROPERTY(BlueprintAssignable, Category = "BB|Events")
	FOnSimSpeedChanged OnSimSpeedChanged;

	UPROPERTY(BlueprintAssignable, Category = "BB|Events")
	FOnEventTriggered OnEventTriggered;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Replay")
	UBBReplayManager* ReplayManager;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	FString SeasonDataPath = TEXT("Data/Season.json");

	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	TSubclassOf<ABBAgentCharacter> AgentCharacterClass;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	TSubclassOf<ABBHouseFloor> HouseFloorClass;

private:
	void LoadSeasonData();
	void SpawnHouseFloor();
	void SpawnAgents();
	void AdvanceTick();
	void ApplySnapshotToAgents(const FBBSnapshot& Snapshot);
	void ProcessEventsAtTick(int32 Tick);
	float GetSpeedMultiplier() const;

	UPROPERTY()
	ABBHouseFloor* HouseFloor;

	UPROPERTY()
	TArray<ABBAgentCharacter*> SpawnedAgents;

	int32 CurrentTick = 0;
	EBBSimSpeed SimSpeed = EBBSimSpeed::Paused;
	float TickAccumulator = 0.f;
	float SecondsPerTick = 2.0f;
};
