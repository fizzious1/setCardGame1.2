// Copyright Big Brother AI Simulator. All Rights Reserved.

#include "BBHouseFloor.h"
#include "BigBrotherSim.h"
#include "BBRoomVolume.h"
#include "Engine/World.h"

ABBHouseFloor::ABBHouseFloor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootSceneComponent;
}

void ABBHouseFloor::BeginPlay()
{
	Super::BeginPlay();
}

void ABBHouseFloor::InitializeFromRoomData(const TArray<FBBRoom>& Rooms)
{
	// Clear existing rooms
	for (ABBRoomVolume* Vol : RoomVolumes)
	{
		if (Vol)
		{
			Vol->Destroy();
		}
	}
	RoomVolumes.Empty();
	RoomMap.Empty();

	for (const FBBRoom& RoomData : Rooms)
	{
		SpawnRoomVolume(RoomData);
	}

	CalculateOverviewCamera();

	UE_LOG(LogBigBrotherSim, Log, TEXT("House initialized with %d rooms."), RoomVolumes.Num());
}

void ABBHouseFloor::SpawnRoomVolume(const FBBRoom& RoomData)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	ABBRoomVolume* Volume = nullptr;
	if (RoomVolumeClass)
	{
		Volume = World->SpawnActor<ABBRoomVolume>(RoomVolumeClass, RoomData.Position, FRotator::ZeroRotator, SpawnParams);
	}
	else
	{
		Volume = World->SpawnActor<ABBRoomVolume>(ABBRoomVolume::StaticClass(), RoomData.Position, FRotator::ZeroRotator, SpawnParams);
	}

	if (Volume)
	{
		Volume->InitializeRoom(RoomData);
		Volume->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		RoomVolumes.Add(Volume);
		RoomMap.Add(RoomData.RoomId, Volume);

		UE_LOG(LogBigBrotherSim, Log, TEXT("Spawned room: %s at (%s)"),
			*RoomData.RoomName, *RoomData.Position.ToString());
	}
}

ABBRoomVolume* ABBHouseFloor::GetRoomById(const FString& RoomId) const
{
	ABBRoomVolume* const* Found = RoomMap.Find(RoomId);
	return Found ? *Found : nullptr;
}

FVector ABBHouseFloor::GetNavPointInRoom(const FString& RoomId) const
{
	ABBRoomVolume* Room = GetRoomById(RoomId);
	if (Room)
	{
		return Room->GetRandomPositionInRoom();
	}
	return FVector::ZeroVector;
}

void ABBHouseFloor::CalculateOverviewCamera()
{
	if (RoomVolumes.Num() == 0)
	{
		return;
	}

	// Calculate bounding box of all rooms
	FVector MinBound = FVector(TNumericLimits<float>::Max());
	FVector MaxBound = FVector(TNumericLimits<float>::Lowest());

	for (const ABBRoomVolume* Vol : RoomVolumes)
	{
		if (!Vol) continue;
		FVector Pos = Vol->GetActorLocation();
		FVector Ext = Vol->GetRoomExtents();

		MinBound.X = FMath::Min(MinBound.X, Pos.X - Ext.X);
		MinBound.Y = FMath::Min(MinBound.Y, Pos.Y - Ext.Y);
		MaxBound.X = FMath::Max(MaxBound.X, Pos.X + Ext.X);
		MaxBound.Y = FMath::Max(MaxBound.Y, Pos.Y + Ext.Y);
	}

	FVector Center = (MinBound + MaxBound) * 0.5f;
	float Span = FMath::Max(MaxBound.X - MinBound.X, MaxBound.Y - MinBound.Y);
	float Height = Span * 1.2f; // Add some margin

	OverviewCameraPosition = FVector(Center.X, Center.Y, FMath::Max(Height, 2000.f));
	OverviewCameraRotation = FRotator(-90.f, 0.f, 0.f);
}
