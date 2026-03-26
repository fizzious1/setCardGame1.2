// Copyright Big Brother AI Simulator. All Rights Reserved.

#include "BBPlayerController.h"
#include "BigBrotherSim.h"
#include "BBGameMode.h"
#include "BBDirectorCamera.h"
#include "BBFollowCamera.h"
#include "BBAgentCharacter.h"
#include "BBHouseFloor.h"
#include "BBReplayManager.h"
#include "BBConfessionalRoom.h"
#include "BBHUD.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ABBPlayerController::ABBPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	CameraMode = EBBCameraMode::Director;
}

void ABBPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Setup enhanced input mapping context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	SpawnCameras();
	SwitchToCameraMode(EBBCameraMode::Director);
}

void ABBPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	if (IA_CameraMove)
		EnhancedInput->BindAction(IA_CameraMove, ETriggerEvent::Triggered, this, &ABBPlayerController::OnCameraMove);
	if (IA_CameraLook)
		EnhancedInput->BindAction(IA_CameraLook, ETriggerEvent::Triggered, this, &ABBPlayerController::OnCameraLook);
	if (IA_CameraZoom)
		EnhancedInput->BindAction(IA_CameraZoom, ETriggerEvent::Triggered, this, &ABBPlayerController::OnCameraZoom);
	if (IA_TogglePlayPause)
		EnhancedInput->BindAction(IA_TogglePlayPause, ETriggerEvent::Started, this, &ABBPlayerController::OnTogglePlayPause);
	if (IA_StepForward)
		EnhancedInput->BindAction(IA_StepForward, ETriggerEvent::Started, this, &ABBPlayerController::OnStepForward);
	if (IA_StepBackward)
		EnhancedInput->BindAction(IA_StepBackward, ETriggerEvent::Started, this, &ABBPlayerController::OnStepBackward);
	if (IA_CameraDirector)
		EnhancedInput->BindAction(IA_CameraDirector, ETriggerEvent::Started, this, &ABBPlayerController::OnCameraDirectorMode);
	if (IA_CameraFollow)
		EnhancedInput->BindAction(IA_CameraFollow, ETriggerEvent::Started, this, &ABBPlayerController::OnCameraFollowMode);
	if (IA_CameraOverview)
		EnhancedInput->BindAction(IA_CameraOverview, ETriggerEvent::Started, this, &ABBPlayerController::OnCameraOverviewMode);
	if (IA_CycleAgent)
		EnhancedInput->BindAction(IA_CycleAgent, ETriggerEvent::Started, this, &ABBPlayerController::OnCycleAgent);
	if (IA_SelectAgent)
		EnhancedInput->BindAction(IA_SelectAgent, ETriggerEvent::Started, this, &ABBPlayerController::OnSelectAgent);
	if (IA_ToggleReplay)
		EnhancedInput->BindAction(IA_ToggleReplay, ETriggerEvent::Started, this, &ABBPlayerController::OnToggleReplay);
	if (IA_Confessional)
		EnhancedInput->BindAction(IA_Confessional, ETriggerEvent::Started, this, &ABBPlayerController::OnConfessional);
	if (IA_SpeedUp)
		EnhancedInput->BindAction(IA_SpeedUp, ETriggerEvent::Started, this, &ABBPlayerController::OnSpeedUp);
	if (IA_SlowDown)
		EnhancedInput->BindAction(IA_SlowDown, ETriggerEvent::Started, this, &ABBPlayerController::OnSlowDown);
}

void ABBPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

EBBCameraMode ABBPlayerController::GetCurrentCameraMode() const
{
	return CameraMode;
}

void ABBPlayerController::SpawnCameras()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (DirectorCameraClass)
	{
		DirectorCamera = World->SpawnActor<ABBDirectorCamera>(DirectorCameraClass, FVector(0.f, 0.f, 1500.f), FRotator(-45.f, 0.f, 0.f), Params);
	}
	else
	{
		DirectorCamera = World->SpawnActor<ABBDirectorCamera>(ABBDirectorCamera::StaticClass(), FVector(0.f, 0.f, 1500.f), FRotator(-45.f, 0.f, 0.f), Params);
	}

	if (FollowCameraClass)
	{
		FollowCamera = World->SpawnActor<ABBFollowCamera>(FollowCameraClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);
	}
	else
	{
		FollowCamera = World->SpawnActor<ABBFollowCamera>(ABBFollowCamera::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	}

	UE_LOG(LogBigBrotherSim, Log, TEXT("Cameras spawned."));
}

void ABBPlayerController::SwitchToCameraMode(EBBCameraMode Mode)
{
	CameraMode = Mode;

	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));

	switch (Mode)
	{
	case EBBCameraMode::Director:
		if (DirectorCamera)
		{
			SetViewTargetWithBlend(DirectorCamera, 0.5f);
			DirectorCamera->SetAutoDirecting(true);
			DirectorCamera->SetManualOverride(false);
		}
		break;

	case EBBCameraMode::Follow:
		if (FollowCamera)
		{
			SetViewTargetWithBlend(FollowCamera, 0.5f);

			// If no target, pick the first agent
			if (!FollowCamera->GetFollowTarget() && GM)
			{
				TArray<ABBAgentCharacter*> Agents = GM->GetAllAgents();
				if (Agents.Num() > 0)
				{
					FollowCamera->SetFollowTarget(Agents[0]);
				}
			}
		}
		break;

	case EBBCameraMode::Overview:
		if (DirectorCamera && GM && GM->GetHouseFloor())
		{
			SetViewTargetWithBlend(DirectorCamera, 0.5f);
			DirectorCamera->SetAutoDirecting(false);
			DirectorCamera->SetManualOverride(false);
			DirectorCamera->SwitchToOverview(
				GM->GetHouseFloor()->GetOverviewCameraPosition(),
				GM->GetHouseFloor()->GetOverviewCameraRotation());
		}
		break;
	}

	// Update HUD
	ABBHUD* BBHud = Cast<ABBHUD>(GetHUD());
	if (BBHud)
	{
		BBHud->SetCameraMode(Mode);
	}

	UE_LOG(LogBigBrotherSim, Log, TEXT("Camera mode: %d"), static_cast<int32>(Mode));
}

void ABBPlayerController::OnCameraMove(const FInputActionValue& Value)
{
	FVector2D MoveVal = Value.Get<FVector2D>();

	if (CameraMode == EBBCameraMode::Director && DirectorCamera && DirectorCamera->IsManualOverride())
	{
		DirectorCamera->AddMovementInput(FVector(MoveVal.Y, MoveVal.X, 0.f), 1.0f);
	}
}

void ABBPlayerController::OnCameraLook(const FInputActionValue& Value)
{
	FVector2D LookVal = Value.Get<FVector2D>();

	if (CameraMode == EBBCameraMode::Director && DirectorCamera && DirectorCamera->IsManualOverride())
	{
		DirectorCamera->AddLookInput(LookVal.X, LookVal.Y);
	}
	else if (CameraMode == EBBCameraMode::Follow && FollowCamera && FollowCamera->IsOrbitMode())
	{
		FollowCamera->AddOrbitInput(LookVal.X, LookVal.Y);
	}
}

void ABBPlayerController::OnCameraZoom(const FInputActionValue& Value)
{
	// Zoom currently handled via spring arm or camera distance adjustments
	// Could be extended for director camera zoom
}

void ABBPlayerController::OnTogglePlayPause(const FInputActionValue& Value)
{
	if (bReplayMode)
	{
		ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
		if (GM && GM->ReplayManager)
		{
			GM->ReplayManager->ToggleReplayPause();
		}
	}
	else
	{
		ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
		if (GM)
		{
			GM->TogglePlayPause();
		}
	}
}

void ABBPlayerController::OnStepForward(const FInputActionValue& Value)
{
	if (bReplayMode)
	{
		ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
		if (GM && GM->ReplayManager)
		{
			GM->ReplayManager->ScrubForward();
		}
	}
	else
	{
		ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
		if (GM)
		{
			GM->StepForward();
		}
	}
}

void ABBPlayerController::OnStepBackward(const FInputActionValue& Value)
{
	if (bReplayMode)
	{
		ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
		if (GM && GM->ReplayManager)
		{
			GM->ReplayManager->ScrubBackward();
		}
	}
	else
	{
		ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
		if (GM)
		{
			GM->StepBackward();
		}
	}
}

void ABBPlayerController::OnCameraDirectorMode(const FInputActionValue& Value)
{
	SwitchToCameraMode(EBBCameraMode::Director);
}

void ABBPlayerController::OnCameraFollowMode(const FInputActionValue& Value)
{
	SwitchToCameraMode(EBBCameraMode::Follow);
}

void ABBPlayerController::OnCameraOverviewMode(const FInputActionValue& Value)
{
	SwitchToCameraMode(EBBCameraMode::Overview);
}

void ABBPlayerController::OnCycleAgent(const FInputActionValue& Value)
{
	if (FollowCamera)
	{
		FollowCamera->SwitchToNextTarget();

		ABBAgentCharacter* NewTarget = FollowCamera->GetFollowTarget();
		ABBHUD* BBHud = Cast<ABBHUD>(GetHUD());
		if (BBHud && NewTarget)
		{
			BBHud->SetSelectedAgent(NewTarget);
		}

		// Auto-switch to follow mode
		if (CameraMode != EBBCameraMode::Follow)
		{
			SwitchToCameraMode(EBBCameraMode::Follow);
		}
	}
}

void ABBPlayerController::OnSelectAgent(const FInputActionValue& Value)
{
	ABBAgentCharacter* HitAgent = TraceForAgent();
	if (HitAgent)
	{
		ABBHUD* BBHud = Cast<ABBHUD>(GetHUD());
		if (BBHud)
		{
			BBHud->SetSelectedAgent(HitAgent);
		}

		if (FollowCamera)
		{
			FollowCamera->SetFollowTarget(HitAgent);
		}

		SwitchToCameraMode(EBBCameraMode::Follow);

		UE_LOG(LogBigBrotherSim, Log, TEXT("Selected agent: %s"), *HitAgent->GetDisplayName());
	}
}

void ABBPlayerController::OnToggleReplay(const FInputActionValue& Value)
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM || !GM->ReplayManager)
	{
		return;
	}

	bReplayMode = !bReplayMode;

	ABBHUD* BBHud = Cast<ABBHUD>(GetHUD());
	if (BBHud)
	{
		BBHud->SetReplayControlsVisible(bReplayMode);
	}

	if (bReplayMode)
	{
		// Start replay from 50 ticks ago to current
		int32 Current = GM->GetCurrentTick();
		int32 Start = FMath::Max(0, Current - 50);
		GM->ReplayManager->StartReplay(Start, Current);
		GM->SetSimSpeed(EBBSimSpeed::Paused);
	}
	else
	{
		GM->ReplayManager->StopReplay();
	}
}

void ABBPlayerController::OnConfessional(const FInputActionValue& Value)
{
	// Find the confessional room and jump camera to it
	TArray<AActor*> ConfessionalRooms;
	UGameplayStatics::GetAllActorsOfClass(this, ABBConfessionalRoom::StaticClass(), ConfessionalRooms);

	if (ConfessionalRooms.Num() > 0)
	{
		ABBConfessionalRoom* Confessional = Cast<ABBConfessionalRoom>(ConfessionalRooms[0]);
		if (Confessional && DirectorCamera)
		{
			UCameraComponent* ConfCam = Confessional->GetConfessionalCamera();
			if (ConfCam)
			{
				SetViewTargetWithBlend(Confessional, 0.5f);
				CameraMode = EBBCameraMode::Director; // Custom mode, but use director enum
				UE_LOG(LogBigBrotherSim, Log, TEXT("Switched to confessional camera."));
			}
		}
	}
}

void ABBPlayerController::OnSpeedUp(const FInputActionValue& Value)
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;

	if (bReplayMode && GM->ReplayManager)
	{
		float CurrentSpeed = GM->ReplayManager->GetReplaySpeed();
		GM->ReplayManager->SetReplaySpeed(CurrentSpeed * 2.f);
	}
	else
	{
		switch (GM->GetSimSpeed())
		{
		case EBBSimSpeed::Paused: GM->SetSimSpeed(EBBSimSpeed::Normal); break;
		case EBBSimSpeed::Normal: GM->SetSimSpeed(EBBSimSpeed::Fast); break;
		case EBBSimSpeed::Fast: GM->SetSimSpeed(EBBSimSpeed::VeryFast); break;
		default: break;
		}
	}
}

void ABBPlayerController::OnSlowDown(const FInputActionValue& Value)
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;

	if (bReplayMode && GM->ReplayManager)
	{
		float CurrentSpeed = GM->ReplayManager->GetReplaySpeed();
		GM->ReplayManager->SetReplaySpeed(CurrentSpeed * 0.5f);
	}
	else
	{
		switch (GM->GetSimSpeed())
		{
		case EBBSimSpeed::VeryFast: GM->SetSimSpeed(EBBSimSpeed::Fast); break;
		case EBBSimSpeed::Fast: GM->SetSimSpeed(EBBSimSpeed::Normal); break;
		case EBBSimSpeed::Normal: GM->SetSimSpeed(EBBSimSpeed::Paused); break;
		default: break;
		}
	}
}

ABBAgentCharacter* ABBPlayerController::TraceForAgent() const
{
	FHitResult HitResult;
	FVector WorldLocation;
	FVector WorldDirection;

	if (DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		FVector TraceEnd = WorldLocation + WorldDirection * 10000.f;
		FCollisionQueryParams QueryParams;
		QueryParams.bTraceComplex = false;
		QueryParams.AddIgnoredActor(this);

		if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Pawn, QueryParams))
		{
			return Cast<ABBAgentCharacter>(HitResult.GetActor());
		}
	}

	return nullptr;
}
