// Copyright Big Brother AI Simulator. All Rights Reserved.

#include "BBConfessionalRoom.h"
#include "BigBrotherSim.h"
#include "BBAgentCharacter.h"
#include "BBGameMode.h"
#include "Camera/CameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"

ABBConfessionalRoom::ABBConfessionalRoom()
{
	PrimaryActorTick.bCanEverTick = true;

	RootSceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootSceneComp;

	// Confessional camera - angled to face the chair
	ConfessionalCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ConfessionalCamera"));
	ConfessionalCamera->SetupAttachment(RootComponent);
	ConfessionalCamera->SetRelativeLocation(FVector(200.f, 0.f, 120.f));
	ConfessionalCamera->SetRelativeRotation(FRotator(-10.f, 180.f, 0.f));
	ConfessionalCamera->FieldOfView = 60.f;

	// Dramatic spotlight from above
	Spotlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Spotlight"));
	Spotlight->SetupAttachment(RootComponent);
	Spotlight->SetRelativeLocation(FVector(0.f, 0.f, 300.f));
	Spotlight->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Spotlight->SetIntensity(0.f); // Off by default
	Spotlight->SetOuterConeAngle(35.f);
	Spotlight->SetInnerConeAngle(25.f);
	Spotlight->SetLightColor(SpotlightColorActive.ToFColor(true));

	// Ambient light
	AmbientLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AmbientLight"));
	AmbientLight->SetupAttachment(RootComponent);
	AmbientLight->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
	AmbientLight->SetIntensity(AmbientIntensityInactive);
	AmbientLight->SetAttenuationRadius(500.f);

	// Subtitle widget for confessional text
	SubtitleWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("SubtitleWidget"));
	SubtitleWidget->SetupAttachment(RootComponent);
	SubtitleWidget->SetRelativeLocation(FVector(200.f, 0.f, 0.f));
	SubtitleWidget->SetWidgetSpace(EWidgetSpace::Screen);
	SubtitleWidget->SetDrawSize(FVector2D(600.f, 100.f));
	SubtitleWidget->SetVisibility(false);

	// Mood/trust overlay widget
	MoodTrustOverlay = CreateDefaultSubobject<UWidgetComponent>(TEXT("MoodTrustOverlay"));
	MoodTrustOverlay->SetupAttachment(RootComponent);
	MoodTrustOverlay->SetRelativeLocation(FVector(200.f, 250.f, 150.f));
	MoodTrustOverlay->SetWidgetSpace(EWidgetSpace::Screen);
	MoodTrustOverlay->SetDrawSize(FVector2D(250.f, 300.f));
	MoodTrustOverlay->SetVisibility(false);
}

void ABBConfessionalRoom::BeginPlay()
{
	Super::BeginPlay();

	// Register for events
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		GM->OnEventTriggered.AddDynamic(this, &ABBConfessionalRoom::OnEventOccurred);
	}
}

void ABBConfessionalRoom::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bConfessionActive)
	{
		UpdateSubtitleDisplay(DeltaTime);
	}
}

void ABBConfessionalRoom::StartConfession(ABBAgentCharacter* Agent, const FBBGameEvent& ConfessionEvent)
{
	if (!Agent)
	{
		return;
	}

	// End any existing confession first
	if (bConfessionActive)
	{
		EndConfession();
	}

	SeatedAgent = Agent;
	CurrentConfessionEvent = ConfessionEvent;
	CurrentConfessionText = ConfessionEvent.DialogueText;
	bConfessionActive = true;
	ConfessionTimer = 0.f;

	// Move agent to chair position
	FVector WorldChairPos = GetActorLocation() + GetActorRotation().RotateVector(ChairPosition);
	FRotator WorldChairRot = GetActorRotation() + ChairRotation;
	Agent->SetActorLocation(WorldChairPos);
	Agent->SetActorRotation(WorldChairRot);
	Agent->SetAgentState(EBBAgentState::Confessing);

	// Activate dramatic lighting
	ActivateDramaticLighting();

	// Show UI overlays
	SubtitleWidget->SetVisibility(true);
	MoodTrustOverlay->SetVisibility(true);

	UE_LOG(LogBigBrotherSim, Log, TEXT("Confessional started: %s - '%s'"),
		*Agent->GetDisplayName(), *CurrentConfessionText.Left(50));
}

void ABBConfessionalRoom::EndConfession()
{
	if (!bConfessionActive)
	{
		return;
	}

	if (SeatedAgent.IsValid())
	{
		SeatedAgent->SetAgentState(EBBAgentState::Idle);
	}

	bConfessionActive = false;
	SeatedAgent = nullptr;
	CurrentConfessionText.Empty();
	ConfessionTimer = 0.f;

	DeactivateDramaticLighting();

	SubtitleWidget->SetVisibility(false);
	MoodTrustOverlay->SetVisibility(false);

	UE_LOG(LogBigBrotherSim, Log, TEXT("Confessional ended."));
}

ABBAgentCharacter* ABBConfessionalRoom::GetSeatedAgent() const
{
	return SeatedAgent.Get();
}

void ABBConfessionalRoom::OnEventOccurred(const FBBGameEvent& Event)
{
	if (Event.EventType != EBBEventType::Confession)
	{
		return;
	}

	// Auto-trigger confessional for confession events
	if (Event.ParticipantIds.Num() > 0)
	{
		ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
		if (GM)
		{
			ABBAgentCharacter* Agent = GM->GetAgentByParticipantId(Event.ParticipantIds[0]);
			if (Agent)
			{
				StartConfession(Agent, Event);
			}
		}
	}
}

void ABBConfessionalRoom::ActivateDramaticLighting()
{
	Spotlight->SetIntensity(SpotlightIntensity);
	Spotlight->SetLightColor(SpotlightColorActive.ToFColor(true));
	AmbientLight->SetIntensity(AmbientIntensityActive);
}

void ABBConfessionalRoom::DeactivateDramaticLighting()
{
	Spotlight->SetIntensity(0.f);
	AmbientLight->SetIntensity(AmbientIntensityInactive);
}

void ABBConfessionalRoom::UpdateSubtitleDisplay(float DeltaTime)
{
	ConfessionTimer += DeltaTime;

	if (ConfessionTimer >= ConfessionDisplayDuration)
	{
		EndConfession();
	}

	// Subtitle text and mood/trust overlay are driven by the bound UMG widgets
	// which read GetConfessionText(), GetSeatedAgent()->GetCurrentMood(), etc.
}
