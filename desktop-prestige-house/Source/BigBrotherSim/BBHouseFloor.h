// Copyright Big Brother AI Simulator. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBSimDataSubsystem.h"
#include "BBHouseFloor.generated.h"

class ABBRoomVolume;

UCLASS()
class BIGBROTHERSIM_API ABBHouseFloor : public AActor
{
	GENERATED_BODY()

public:
	ABBHouseFloor();

	virtual void BeginPlay() override;

	/** Initialize rooms from season data */
	UFUNCTION(BlueprintCallable, Category = "BB|House")
	void InitializeFromRoomData(const TArray<FBBRoom>& Rooms);

	/** Get a room volume by room ID */
	UFUNCTION(BlueprintCallable, Category = "BB|House")
	ABBRoomVolume* GetRoomById(const FString& RoomId) const;

	/** Get all room volumes */
	UFUNCTION(BlueprintCallable, Category = "BB|House")
	TArray<ABBRoomVolume*> GetAllRoomVolumes() const { return RoomVolumes; }

	/** Get a navigation point inside a room */
	UFUNCTION(BlueprintCallable, Category = "BB|House")
	FVector GetNavPointInRoom(const FString& RoomId) const;

	/** Get the overview camera position */
	UFUNCTION(BlueprintPure, Category = "BB|House")
	FVector GetOverviewCameraPosition() const { return OverviewCameraPosition; }

	/** Get the overview camera rotation */
	UFUNCTION(BlueprintPure, Category = "BB|House")
	FRotator GetOverviewCameraRotation() const { return OverviewCameraRotation; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Components")
	USceneComponent* RootSceneComponent;

	UPROPERTY(EditDefaultsOnly, Category = "BB|Config")
	TSubclassOf<ABBRoomVolume> RoomVolumeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BB|Camera")
	FVector OverviewCameraPosition = FVector(0.f, 0.f, 3000.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BB|Camera")
	FRotator OverviewCameraRotation = FRotator(-90.f, 0.f, 0.f);

private:
	void SpawnRoomVolume(const FBBRoom& RoomData);
	void CalculateOverviewCamera();

	UPROPERTY()
	TArray<ABBRoomVolume*> RoomVolumes;

	TMap<FString, ABBRoomVolume*> RoomMap;
};
