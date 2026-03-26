// Copyright Big Brother AI Simulator. All Rights Reserved.

#include "BBRoomVolume.h"
#include "BigBrotherSim.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"

ABBRoomVolume::ABBRoomVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootSceneComponent;

	// Box collision for room boundaries
	RoomBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("RoomBounds"));
	RoomBounds->SetupAttachment(RootComponent);
	RoomBounds->SetCollisionProfileName(TEXT("RoomVolume"));
	RoomBounds->SetGenerateOverlapEvents(true);
	RoomBounds->SetBoxExtent(FVector(500.f, 500.f, 300.f));
	RoomBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// Room name label
	RoomNameLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RoomNameLabel"));
	RoomNameLabel->SetupAttachment(RootComponent);
	RoomNameLabel->SetRelativeLocation(FVector(0.f, 0.f, 280.f));
	RoomNameLabel->SetHorizontalAlignment(EHTA_Center);
	RoomNameLabel->SetVerticalAlignment(EVRTA_TextCenter);
	RoomNameLabel->SetWorldSize(40.f);
	RoomNameLabel->SetTextRenderColor(FColor::White);
	RoomNameLabel->SetText(FText::FromString(TEXT("Room")));
}

void ABBRoomVolume::BeginPlay()
{
	Super::BeginPlay();
}

void ABBRoomVolume::InitializeRoom(const FBBRoom& InRoomData)
{
	RoomData = InRoomData;

	// Update box extent
	RoomBounds->SetBoxExtent(RoomData.Extents);

	// Update label
	RoomNameLabel->SetText(FText::FromString(RoomData.RoomName));

	// Generate navigation points based on capacity
	GenerateNavigationPoints();

	UE_LOG(LogBigBrotherSim, Log, TEXT("Room initialized: %s (Type: %d, Capacity: %d)"),
		*RoomData.RoomName, static_cast<int32>(RoomData.RoomType), RoomData.Capacity);
}

FVector ABBRoomVolume::GetRandomPositionInRoom() const
{
	if (NavPoints.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, NavPoints.Num() - 1);
		return NavPoints[RandomIndex];
	}

	// Fallback: random position within bounds
	FVector Pos = GetActorLocation();
	FVector Ext = RoomData.Extents * 0.8f; // 80% of room for safety margin

	return FVector(
		Pos.X + FMath::RandRange(-Ext.X, Ext.X),
		Pos.Y + FMath::RandRange(-Ext.Y, Ext.Y),
		Pos.Z
	);
}

FVector ABBRoomVolume::GetEventAnchorPoint() const
{
	// Center of the room, slightly raised for visual effects
	FVector Pos = GetActorLocation();
	return FVector(Pos.X, Pos.Y, Pos.Z + 100.f);
}

void ABBRoomVolume::GenerateNavigationPoints()
{
	NavPoints.Empty();

	FVector RoomCenter = GetActorLocation();
	FVector Ext = RoomData.Extents * 0.7f; // 70% of room for nav points

	int32 NumPoints = FMath::Max(RoomData.Capacity, 4);

	if (NumPoints <= 4)
	{
		// Simple grid for small rooms
		NavPoints.Add(RoomCenter + FVector(-Ext.X * 0.5f, -Ext.Y * 0.5f, 0.f));
		NavPoints.Add(RoomCenter + FVector(Ext.X * 0.5f, -Ext.Y * 0.5f, 0.f));
		NavPoints.Add(RoomCenter + FVector(-Ext.X * 0.5f, Ext.Y * 0.5f, 0.f));
		NavPoints.Add(RoomCenter + FVector(Ext.X * 0.5f, Ext.Y * 0.5f, 0.f));
	}
	else
	{
		// Distribute points around the room perimeter and center
		// Center point
		NavPoints.Add(RoomCenter);

		// Ring of points around center
		float Radius = FMath::Min(Ext.X, Ext.Y) * 0.6f;
		int32 RingPoints = NumPoints - 1;
		float AngleStep = 360.f / static_cast<float>(RingPoints);

		for (int32 i = 0; i < RingPoints; ++i)
		{
			float AngleRad = FMath::DegreesToRadians(AngleStep * i);
			FVector Offset(
				FMath::Cos(AngleRad) * Radius,
				FMath::Sin(AngleRad) * Radius,
				0.f
			);
			NavPoints.Add(RoomCenter + Offset);
		}
	}

	UE_LOG(LogBigBrotherSim, Verbose, TEXT("Generated %d nav points for room %s"),
		NavPoints.Num(), *RoomData.RoomName);
}
