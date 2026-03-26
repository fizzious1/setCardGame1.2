// Copyright Big Brother AI Simulator. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBSimDataSubsystem.h"
#include "BBRoomVolume.generated.h"

class UBoxComponent;
class UTextRenderComponent;

UCLASS()
class BIGBROTHERSIM_API ABBRoomVolume : public AActor
{
	GENERATED_BODY()

public:
	ABBRoomVolume();

	virtual void BeginPlay() override;

	/** Initialize from room data */
	UFUNCTION(BlueprintCallable, Category = "BB|Room")
	void InitializeRoom(const FBBRoom& InRoomData);

	/** Get a random navigable position inside this room */
	UFUNCTION(BlueprintCallable, Category = "BB|Room")
	FVector GetRandomPositionInRoom() const;

	/** Get room ID */
	UFUNCTION(BlueprintPure, Category = "BB|Room")
	FString GetRoomId() const { return RoomData.RoomId; }

	/** Get room name */
	UFUNCTION(BlueprintPure, Category = "BB|Room")
	FString GetRoomName() const { return RoomData.RoomName; }

	/** Get room type */
	UFUNCTION(BlueprintPure, Category = "BB|Room")
	EBBRoomType GetRoomType() const { return RoomData.RoomType; }

	/** Get room capacity */
	UFUNCTION(BlueprintPure, Category = "BB|Room")
	int32 GetRoomCapacity() const { return RoomData.Capacity; }

	/** Get room extents */
	UFUNCTION(BlueprintPure, Category = "BB|Room")
	FVector GetRoomExtents() const { return RoomData.Extents; }

	/** Get navigation points for agent spawning inside the room */
	UFUNCTION(BlueprintCallable, Category = "BB|Room")
	TArray<FVector> GetNavigationPoints() const { return NavPoints; }

	/** Get event visualization anchor point */
	UFUNCTION(BlueprintPure, Category = "BB|Room")
	FVector GetEventAnchorPoint() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Components")
	UBoxComponent* RoomBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Components")
	UTextRenderComponent* RoomNameLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BB|Components")
	USceneComponent* RootSceneComponent;

private:
	void GenerateNavigationPoints();

	FBBRoom RoomData;
	TArray<FVector> NavPoints;
};
