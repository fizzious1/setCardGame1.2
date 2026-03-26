// Copyright Big Brother AI Simulator. All Rights Reserved.

#include "BBHUD.h"
#include "BigBrotherSim.h"
#include "BBAgentCharacter.h"
#include "BBGameMode.h"
#include "BBReplayManager.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"

ABBHUD::ABBHUD()
{
}

void ABBHUD::BeginPlay()
{
	Super::BeginPlay();

	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		GM->OnEventTriggered.AddDynamic(this, &ABBHUD::OnEventTriggered);
	}
}

void ABBHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	float DeltaTime = GetWorld()->GetDeltaSeconds();

	DrawDayHourDisplay();
	DrawCameraModeIndicator();
	DrawMiniMap();
	DrawSelectedAgentInfo();
	DrawActiveEventNotification();
	DrawEventPopup(DeltaTime);

	if (bReplayControlsVisible)
	{
		DrawReplayControls();
	}
}

void ABBHUD::SetSelectedAgent(ABBAgentCharacter* Agent)
{
	SelectedAgent = Agent;
}

ABBAgentCharacter* ABBHUD::GetSelectedAgent() const
{
	return SelectedAgent.Get();
}

void ABBHUD::SetCameraMode(EBBCameraMode Mode)
{
	CurrentCameraMode = Mode;
}

void ABBHUD::ShowEventPopup(const FString& Text, const FLinearColor& Color)
{
	PopupText = Text;
	PopupColor = Color;
	PopupTimer = EventPopupDuration;
	bPopupActive = true;
}

void ABBHUD::SetReplayControlsVisible(bool bVisible)
{
	bReplayControlsVisible = bVisible;
}

void ABBHUD::OnEventTriggered(const FBBGameEvent& Event)
{
	// Show popup for high-drama events
	if (Event.DramaScore > 0.3f)
	{
		FLinearColor EvtColor = FLinearColor::White;
		FString EvtText = Event.Description;

		switch (Event.EventType)
		{
		case EBBEventType::Alliance: EvtColor = FLinearColor(0.1f, 0.9f, 0.2f); break;
		case EBBEventType::Conflict: EvtColor = FLinearColor(0.9f, 0.1f, 0.1f); break;
		case EBBEventType::Betrayal: EvtColor = FLinearColor(0.6f, 0.0f, 0.8f); break;
		case EBBEventType::Nomination: EvtColor = FLinearColor(1.0f, 0.5f, 0.0f); break;
		case EBBEventType::Eviction: EvtColor = FLinearColor(0.5f, 0.5f, 0.5f); break;
		default: break;
		}

		ShowEventPopup(EvtText, EvtColor);
	}
}

void ABBHUD::DrawDayHourDisplay()
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return;
	}

	UBBSimDataSubsystem* DataSub = GM->GetGameInstance()->GetSubsystem<UBBSimDataSubsystem>();
	if (!DataSub || !DataSub->IsSeasonLoaded())
	{
		return;
	}

	FBBSnapshot Snap = DataSub->GetSnapshotAtTick(GM->GetCurrentTick());

	FString DayText = FString::Printf(TEXT("Day %d - %d:00"), Snap.Day, Snap.Hour);
	FString TickText = FString::Printf(TEXT("Tick: %d / %d"), GM->GetCurrentTick(), DataSub->GetTotalTicks());

	FString SpeedText;
	switch (GM->GetSimSpeed())
	{
	case EBBSimSpeed::Paused: SpeedText = TEXT("PAUSED"); break;
	case EBBSimSpeed::Normal: SpeedText = TEXT("1x"); break;
	case EBBSimSpeed::Fast: SpeedText = TEXT("2x"); break;
	case EBBSimSpeed::VeryFast: SpeedText = TEXT("4x"); break;
	}

	// Draw in top-left
	float X = 20.f;
	float Y = 20.f;

	// Background box
	FLinearColor BoxColor(0.f, 0.f, 0.f, 0.6f);
	DrawRect(BoxColor, X - 5.f, Y - 5.f, 250.f, 80.f);

	DrawText(DayText, FLinearColor::White, X, Y, HUDFont, 1.2f);
	DrawText(TickText, FLinearColor(0.7f, 0.7f, 0.7f), X, Y + 25.f, HUDFont, 0.9f);
	DrawText(SpeedText, GM->IsPlaying() ? FLinearColor::Green : FLinearColor::Yellow, X, Y + 48.f, HUDFont, 1.0f);
}

void ABBHUD::DrawSelectedAgentInfo()
{
	if (!SelectedAgent.IsValid())
	{
		return;
	}

	ABBAgentCharacter* Agent = SelectedAgent.Get();
	const FBBParticipant& Data = Agent->GetParticipantData();
	const FBBParticipantState& State = Agent->GetCurrentParticipantState();

	float PanelW = 280.f;
	float PanelH = 200.f;
	float X = Canvas->SizeX - PanelW - 20.f;
	float Y = 20.f;

	// Background
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.7f), X, Y, PanelW, PanelH);

	// Name
	DrawText(Data.DisplayName, FLinearColor::White, X + 10.f, Y + 10.f, HUDFont, 1.3f);

	// Info
	FString MoodStr;
	switch (State.Mood)
	{
	case EBBMood::Happy: MoodStr = TEXT("Happy"); break;
	case EBBMood::Sad: MoodStr = TEXT("Sad"); break;
	case EBBMood::Angry: MoodStr = TEXT("Angry"); break;
	case EBBMood::Anxious: MoodStr = TEXT("Anxious"); break;
	case EBBMood::Excited: MoodStr = TEXT("Excited"); break;
	case EBBMood::Scheming: MoodStr = TEXT("Scheming"); break;
	case EBBMood::Betrayed: MoodStr = TEXT("Betrayed"); break;
	default: MoodStr = TEXT("Neutral"); break;
	}

	float LineY = Y + 40.f;
	float LineSpacing = 22.f;

	DrawText(FString::Printf(TEXT("Age: %d | %s"), Data.Age, *Data.Occupation),
		FLinearColor(0.8f, 0.8f, 0.8f), X + 10.f, LineY, HUDFont, 0.85f);
	LineY += LineSpacing;

	DrawText(FString::Printf(TEXT("Mood: %s (%.0f%%)"), *MoodStr, State.MoodIntensity * 100.f),
		FLinearColor(0.9f, 0.9f, 0.3f), X + 10.f, LineY, HUDFont, 0.85f);
	LineY += LineSpacing;

	DrawText(FString::Printf(TEXT("Stress: %.0f%%"), State.StressLevel * 100.f),
		FLinearColor(0.9f, 0.4f, 0.4f), X + 10.f, LineY, HUDFont, 0.85f);
	LineY += LineSpacing;

	DrawText(FString::Printf(TEXT("Room: %s"), *State.CurrentRoomId),
		FLinearColor(0.7f, 0.7f, 0.9f), X + 10.f, LineY, HUDFont, 0.85f);
	LineY += LineSpacing;

	if (State.bIsHeadOfHousehold)
	{
		DrawText(TEXT("HEAD OF HOUSEHOLD"), FLinearColor(1.f, 0.8f, 0.f), X + 10.f, LineY, HUDFont, 0.9f);
		LineY += LineSpacing;
	}
	if (State.bIsNominated)
	{
		DrawText(TEXT("NOMINATED"), FLinearColor(1.f, 0.3f, 0.f), X + 10.f, LineY, HUDFont, 0.9f);
		LineY += LineSpacing;
	}

	// Stats bars
	LineY += 5.f;
	float BarW = 100.f;
	float BarH = 8.f;
	float StatX = X + 100.f;

	auto DrawStatBar = [&](const FString& Label, float Value, const FLinearColor& Color)
	{
		DrawText(Label, FLinearColor(0.7f, 0.7f, 0.7f), X + 10.f, LineY, HUDFont, 0.7f);
		DrawRect(FLinearColor(0.2f, 0.2f, 0.2f), StatX, LineY + 4.f, BarW, BarH);
		DrawRect(Color, StatX, LineY + 4.f, BarW * Value, BarH);
		LineY += 15.f;
	};

	DrawStatBar(TEXT("Charisma"), Data.Charisma, FLinearColor(0.2f, 0.8f, 0.2f));
	DrawStatBar(TEXT("Intel"), Data.Intelligence, FLinearColor(0.2f, 0.5f, 0.9f));
	DrawStatBar(TEXT("Physical"), Data.Physical, FLinearColor(0.9f, 0.5f, 0.2f));
}

void ABBHUD::DrawMiniMap()
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return;
	}

	float MapX = Canvas->SizeX - MiniMapSize - MiniMapPadding;
	float MapY = Canvas->SizeY - MiniMapSize - MiniMapPadding;

	// Background
	DrawRect(FLinearColor(0.05f, 0.05f, 0.1f, 0.8f), MapX, MapY, MiniMapSize, MiniMapSize);

	// Border
	DrawLine(MapX, MapY, MapX + MiniMapSize, MapY, FLinearColor(0.3f, 0.3f, 0.5f));
	DrawLine(MapX + MiniMapSize, MapY, MapX + MiniMapSize, MapY + MiniMapSize, FLinearColor(0.3f, 0.3f, 0.5f));
	DrawLine(MapX + MiniMapSize, MapY + MiniMapSize, MapX, MapY + MiniMapSize, FLinearColor(0.3f, 0.3f, 0.5f));
	DrawLine(MapX, MapY + MiniMapSize, MapX, MapY, FLinearColor(0.3f, 0.3f, 0.5f));

	// Draw agent dots
	TArray<ABBAgentCharacter*> Agents = GM->GetAllAgents();
	for (ABBAgentCharacter* Agent : Agents)
	{
		if (!Agent)
		{
			continue;
		}

		FVector2D MapPos = WorldToMiniMap(Agent->GetActorLocation());
		float DotX = MapX + MapPos.X * MiniMapSize;
		float DotY = MapY + MapPos.Y * MiniMapSize;

		// Clamp to mini-map bounds
		DotX = FMath::Clamp(DotX, MapX, MapX + MiniMapSize);
		DotY = FMath::Clamp(DotY, MapY, MapY + MiniMapSize);

		FLinearColor DotColor = FLinearColor::White;
		if (SelectedAgent.IsValid() && SelectedAgent.Get() == Agent)
		{
			DotColor = FLinearColor(0.f, 1.f, 1.f); // Cyan for selected
			DrawRect(DotColor, DotX - AgentDotSize, DotY - AgentDotSize, AgentDotSize * 2.f, AgentDotSize * 2.f);
		}
		else
		{
			DrawRect(DotColor, DotX - AgentDotSize * 0.5f, DotY - AgentDotSize * 0.5f, AgentDotSize, AgentDotSize);
		}
	}

	// Label
	DrawText(TEXT("HOUSE MAP"), FLinearColor(0.6f, 0.6f, 0.8f), MapX, MapY - 18.f, HUDFont, 0.7f);
}

void ABBHUD::DrawCameraModeIndicator()
{
	FString ModeText;
	switch (CurrentCameraMode)
	{
	case EBBCameraMode::Director: ModeText = TEXT("CAM: Director [1]"); break;
	case EBBCameraMode::Follow: ModeText = TEXT("CAM: Follow [2]"); break;
	case EBBCameraMode::Overview: ModeText = TEXT("CAM: Overview [3]"); break;
	}

	float X = Canvas->SizeX * 0.5f - 80.f;
	float Y = 20.f;

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), X - 5.f, Y - 3.f, 170.f, 25.f);
	DrawText(ModeText, FLinearColor(0.3f, 0.8f, 1.f), X, Y, HUDFont, 0.9f);
}

void ABBHUD::DrawEventPopup(float DeltaTime)
{
	if (!bPopupActive)
	{
		return;
	}

	PopupTimer -= DeltaTime;
	if (PopupTimer <= 0.f)
	{
		bPopupActive = false;
		return;
	}

	float Alpha = FMath::Clamp(PopupTimer / EventPopupDuration, 0.f, 1.f);
	if (PopupTimer < 1.0f)
	{
		Alpha = PopupTimer; // Fade out in last second
	}

	float CenterX = Canvas->SizeX * 0.5f;
	float Y = Canvas->SizeY * 0.25f;

	float TextW = PopupText.Len() * 10.f; // Approximate
	float BoxX = CenterX - TextW * 0.5f - 20.f;
	float BoxW = TextW + 40.f;

	FLinearColor BgColor(0.f, 0.f, 0.f, 0.7f * Alpha);
	DrawRect(BgColor, BoxX, Y - 10.f, BoxW, 50.f);

	FLinearColor TextColor = PopupColor;
	TextColor.A = Alpha;
	DrawText(PopupText, TextColor, CenterX - TextW * 0.5f, Y, HUDFont, 1.5f);
}

void ABBHUD::DrawReplayControls()
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM || !GM->ReplayManager)
	{
		return;
	}

	UBBReplayManager* Replay = GM->ReplayManager;

	float BarW = 400.f;
	float BarH = 6.f;
	float X = (Canvas->SizeX - BarW) * 0.5f;
	float Y = Canvas->SizeY - 80.f;

	// Background
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.7f), X - 10.f, Y - 30.f, BarW + 20.f, 70.f);

	// Replay label
	FString StateStr;
	switch (Replay->GetReplayState())
	{
	case EBBReplayState::Playing: StateStr = TEXT("REPLAY PLAYING"); break;
	case EBBReplayState::Paused: StateStr = TEXT("REPLAY PAUSED"); break;
	default: StateStr = TEXT("REPLAY"); break;
	}

	DrawText(StateStr, FLinearColor(1.f, 0.4f, 0.4f), X, Y - 25.f, HUDFont, 0.8f);
	DrawText(FString::Printf(TEXT("%.0fx"), Replay->GetReplaySpeed()), FLinearColor::White, X + BarW - 30.f, Y - 25.f, HUDFont, 0.8f);

	// Progress bar
	DrawRect(FLinearColor(0.3f, 0.3f, 0.3f), X, Y, BarW, BarH);

	int32 Range = Replay->GetReplayEndTick() - Replay->GetReplayStartTick();
	if (Range > 0)
	{
		float Progress = static_cast<float>(Replay->GetCurrentReplayTick() - Replay->GetReplayStartTick()) / static_cast<float>(Range);
		DrawRect(FLinearColor(1.f, 0.3f, 0.3f), X, Y, BarW * Progress, BarH);
	}

	// Tick display
	FString TickRange = FString::Printf(TEXT("%d / %d - %d"),
		Replay->GetCurrentReplayTick(), Replay->GetReplayStartTick(), Replay->GetReplayEndTick());
	DrawText(TickRange, FLinearColor(0.7f, 0.7f, 0.7f), X, Y + 12.f, HUDFont, 0.7f);

	// Controls hint
	DrawText(TEXT("[R] Stop  [Space] Pause  [<][>] Scrub  [+][-] Speed"),
		FLinearColor(0.5f, 0.5f, 0.5f), X, Y + 28.f, HUDFont, 0.6f);
}

void ABBHUD::DrawActiveEventNotification()
{
	ABBGameMode* GM = Cast<ABBGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return;
	}

	UBBSimDataSubsystem* DataSub = GM->GetGameInstance()->GetSubsystem<UBBSimDataSubsystem>();
	if (!DataSub || !DataSub->IsSeasonLoaded())
	{
		return;
	}

	TArray<FBBGameEvent> Events = DataSub->GetEventsAtTick(GM->GetCurrentTick());
	if (Events.Num() == 0)
	{
		return;
	}

	float X = 20.f;
	float Y = 110.f;

	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), X - 5.f, Y - 5.f, 300.f, 20.f * FMath::Min(Events.Num(), 3) + 10.f);

	int32 Shown = 0;
	for (const FBBGameEvent& Evt : Events)
	{
		if (Shown >= 3) break;

		FLinearColor EvtColor = FLinearColor::White;
		switch (Evt.EventType)
		{
		case EBBEventType::Alliance: EvtColor = FLinearColor(0.1f, 0.9f, 0.2f); break;
		case EBBEventType::Conflict: EvtColor = FLinearColor(0.9f, 0.1f, 0.1f); break;
		case EBBEventType::Betrayal: EvtColor = FLinearColor(0.6f, 0.0f, 0.8f); break;
		case EBBEventType::Eviction: EvtColor = FLinearColor(0.5f, 0.5f, 0.5f); break;
		default: break;
		}

		FString EvtText = Evt.Description.Left(40);
		DrawText(EvtText, EvtColor, X, Y + Shown * 20.f, HUDFont, 0.75f);
		Shown++;
	}
}

FVector2D ABBHUD::WorldToMiniMap(const FVector& WorldPos) const
{
	float NormX = (WorldPos.X - MiniMapWorldMin.X) / (MiniMapWorldMax.X - MiniMapWorldMin.X);
	float NormY = (WorldPos.Y - MiniMapWorldMin.Y) / (MiniMapWorldMax.Y - MiniMapWorldMin.Y);
	return FVector2D(FMath::Clamp(NormX, 0.f, 1.f), FMath::Clamp(NormY, 0.f, 1.f));
}
