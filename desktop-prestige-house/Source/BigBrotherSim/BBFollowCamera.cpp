// Copyright Big Brother AI Simulator. All Rights Reserved.

#include "BBFollowCamera.h"
#include "BigBrotherSim.h"
#include "BBAgentCharacter.h"
#include "BBGameMode.h"
#include "BBHouseFloor.h"
#include "BBRoomVolume.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

ABBFollowCamera::ABBFollowCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	RootComponent = SpringArm;
	SpringArm->TargetArmLength = DefaultArmLength;
	SpringArm->bDoCollisionTest = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = FollowLagSpeed;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 8.f;
	SpringArm->SetRelativeRotation(FRotator(-20.f, 0.f, 0.f));

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArm);
	CameraComp->FieldOfView = 70.f;
}

void ABBFollowCamera::BeginPlay()
{
	Super::BeginPlay();
}

void ABBFollowCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (FollowTarget.IsValid())
	{
		UpdateFollowPosition(DeltaTime);
	}
}

void ABBFollowCamera::SetFollowTarget(ABBAgentCharacter* NewTarget)
{
	FollowTarget = NewTarget;

	if (NewTarget)
	{
		SetActorLocation(NewTarget->GetActorLocation());
		AdjustArmLengthForRoom();
		UE_LOG(LogBigBrotherSim, Log, TEXT("Follow camera targeting: %s"), *NewTarget->GetDisplayName());
	}
}

ABBAgentCharacter* ABBFollowCamera::GetFollowTarget() const
{
	return FollowTarget.Get();
}

void ABBFollowCamera::SwitchToNextTarget()
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return;
	}

	TArray<ABBAgentCharacter*> Agents = GM->GetAllAgents();
	if (Agents.Num() == 0)
	{
		return;
	}

	CurrentTargetIndex = (CurrentTargetIndex + 1) % Agents.Num();
	SetFollowTarget(Agents[CurrentTargetIndex]);
}

void ABBFollowCamera::SwitchToPreviousTarget()
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return;
	}

	TArray<ABBAgentCharacter*> Agents = GM->GetAllAgents();
	if (Agents.Num() == 0)
	{
		return;
	}

	CurrentTargetIndex = (CurrentTargetIndex - 1 + Agents.Num()) % Agents.Num();
	SetFollowTarget(Agents[CurrentTargetIndex]);
}

void ABBFollowCamera::SetOrbitMode(bool bEnable)
{
	bOrbitMode = bEnable;
	if (!bEnable)
	{
		// Reset orbit angles
		OrbitYaw = 0.f;
		OrbitPitch = -20.f;
	}
}

void ABBFollowCamera::AddOrbitInput(float YawInput, float PitchInput)
{
	if (bOrbitMode)
	{
		OrbitYaw += YawInput * OrbitSpeed * GetWorld()->GetDeltaSeconds();
		OrbitPitch = FMath::Clamp(OrbitPitch + PitchInput * OrbitSpeed * GetWorld()->GetDeltaSeconds(), -80.f, 10.f);
	}
}

void ABBFollowCamera::UpdateFollowPosition(float DeltaTime)
{
	if (!FollowTarget.IsValid())
	{
		return;
	}

	FVector TargetLoc = FollowTarget->GetActorLocation();

	if (bOrbitMode)
	{
		// Orbit mode: spring arm rotates around the target
		SpringArm->SetRelativeRotation(FRotator(OrbitPitch, OrbitYaw, 0.f));
		FVector NewLoc = FMath::VInterpTo(GetActorLocation(), TargetLoc, DeltaTime, FollowLagSpeed);
		SetActorLocation(NewLoc);
	}
	else
	{
		// Standard follow: smooth lag behind the character
		FVector NewLoc = FMath::VInterpTo(GetActorLocation(), TargetLoc, DeltaTime, FollowLagSpeed);
		SetActorLocation(NewLoc);

		// Face the direction the agent is facing
		FRotator AgentRot = FollowTarget->GetActorRotation();
		FRotator DesiredRot = FRotator(-20.f, AgentRot.Yaw, 0.f);
		FRotator CurrentRot = SpringArm->GetRelativeRotation();
		FRotator NewRot = FMath::RInterpTo(CurrentRot, DesiredRot, DeltaTime, 3.f);
		SpringArm->SetRelativeRotation(NewRot);
	}
}

void ABBFollowCamera::AdjustArmLengthForRoom()
{
	if (!FollowTarget.IsValid())
	{
		return;
	}

	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM || !GM->GetHouseFloor())
	{
		SpringArm->TargetArmLength = DefaultArmLength;
		return;
	}

	FString RoomId = FollowTarget->GetCurrentRoomId();
	ABBRoomVolume* Room = GM->GetHouseFloor()->GetRoomById(RoomId);

	if (Room)
	{
		FVector Ext = Room->GetRoomExtents();
		float RoomSize = FMath::Min(Ext.X, Ext.Y);
		float DesiredArm = RoomSize * RoomSizeArmMultiplier;
		SpringArm->TargetArmLength = FMath::Clamp(DesiredArm, MinArmLength, MaxArmLength);
	}
	else
	{
		SpringArm->TargetArmLength = DefaultArmLength;
	}
}
