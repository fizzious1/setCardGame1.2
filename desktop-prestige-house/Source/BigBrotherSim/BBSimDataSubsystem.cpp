// Copyright Big Brother AI Simulator. All Rights Reserved.

#include "BBSimDataSubsystem.h"
#include "BigBrotherSim.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void UBBSimDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bSeasonLoaded = false;
	UE_LOG(LogBigBrotherSim, Log, TEXT("BBSimDataSubsystem initialized."));
}

void UBBSimDataSubsystem::Deinitialize()
{
	CachedSeason = FBBSeason();
	TickToSnapshotIndex.Empty();
	TickToEventIndices.Empty();
	bSeasonLoaded = false;
	UE_LOG(LogBigBrotherSim, Log, TEXT("BBSimDataSubsystem deinitialized."));
	Super::Deinitialize();
}

bool UBBSimDataSubsystem::LoadSeasonFromFile(const FString& FilePath)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		UE_LOG(LogBigBrotherSim, Error, TEXT("Failed to load season file: %s"), *FilePath);
		return false;
	}
	return LoadSeasonFromString(JsonString);
}

bool UBBSimDataSubsystem::LoadSeasonFromString(const FString& JsonString)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogBigBrotherSim, Error, TEXT("Failed to parse season JSON."));
		return false;
	}

	return ParseSeasonJson(JsonObject);
}

bool UBBSimDataSubsystem::ParseSeasonJson(TSharedPtr<FJsonObject> JsonObject)
{
	CachedSeason = FBBSeason();
	TickToSnapshotIndex.Empty();
	TickToEventIndices.Empty();

	CachedSeason.SeasonId = JsonObject->GetStringField(TEXT("seasonId"));
	CachedSeason.SeasonName = JsonObject->GetStringField(TEXT("seasonName"));
	CachedSeason.TotalDays = JsonObject->GetIntegerField(TEXT("totalDays"));
	CachedSeason.TotalTicks = JsonObject->GetIntegerField(TEXT("totalTicks"));

	// Parse participants
	const TArray<TSharedPtr<FJsonValue>>* ParticipantsArray;
	if (JsonObject->TryGetArrayField(TEXT("participants"), ParticipantsArray))
	{
		for (const auto& Val : *ParticipantsArray)
		{
			CachedSeason.Participants.Add(ParseParticipant(Val->AsObject()));
		}
	}

	// Parse rooms
	const TArray<TSharedPtr<FJsonValue>>* RoomsArray;
	if (JsonObject->TryGetArrayField(TEXT("rooms"), RoomsArray))
	{
		for (const auto& Val : *RoomsArray)
		{
			CachedSeason.Rooms.Add(ParseRoom(Val->AsObject()));
		}
	}

	// Parse snapshots and build tick index
	const TArray<TSharedPtr<FJsonValue>>* SnapshotsArray;
	if (JsonObject->TryGetArrayField(TEXT("snapshots"), SnapshotsArray))
	{
		for (int32 i = 0; i < SnapshotsArray->Num(); ++i)
		{
			FBBSnapshot Snap = ParseSnapshot((*SnapshotsArray)[i]->AsObject());
			TickToSnapshotIndex.Add(Snap.Tick, i);
			CachedSeason.Snapshots.Add(Snap);
		}
	}

	// Parse events and build tick index
	const TArray<TSharedPtr<FJsonValue>>* EventsArray;
	if (JsonObject->TryGetArrayField(TEXT("events"), EventsArray))
	{
		for (int32 i = 0; i < EventsArray->Num(); ++i)
		{
			FBBGameEvent Evt = ParseGameEvent((*EventsArray)[i]->AsObject());
			TickToEventIndices.FindOrAdd(Evt.Tick).Add(i);
			CachedSeason.Events.Add(Evt);
		}
	}

	// Parse daily recaps
	const TArray<TSharedPtr<FJsonValue>>* RecapsArray;
	if (JsonObject->TryGetArrayField(TEXT("dailyRecaps"), RecapsArray))
	{
		for (const auto& Val : *RecapsArray)
		{
			CachedSeason.DailyRecaps.Add(ParseDailyRecap(Val->AsObject()));
		}
	}

	// Parse highlights
	const TArray<TSharedPtr<FJsonValue>>* HighlightsArray;
	if (JsonObject->TryGetArrayField(TEXT("highlights"), HighlightsArray))
	{
		for (const auto& Val : *HighlightsArray)
		{
			CachedSeason.Highlights.Add(ParseHighlight(Val->AsObject()));
		}
	}

	bSeasonLoaded = true;
	UE_LOG(LogBigBrotherSim, Log, TEXT("Season '%s' loaded: %d participants, %d snapshots, %d events"),
		*CachedSeason.SeasonName, CachedSeason.Participants.Num(),
		CachedSeason.Snapshots.Num(), CachedSeason.Events.Num());
	return true;
}

FBBParticipant UBBSimDataSubsystem::ParseParticipant(const TSharedPtr<FJsonObject>& Obj) const
{
	FBBParticipant P;
	P.ParticipantId = Obj->GetStringField(TEXT("participantId"));
	P.DisplayName = Obj->GetStringField(TEXT("displayName"));
	P.Age = Obj->GetIntegerField(TEXT("age"));
	P.Occupation = Obj->GetStringField(TEXT("occupation"));
	P.Bio = Obj->GetStringField(TEXT("bio"));
	P.AvatarAssetPath = Obj->GetStringField(TEXT("avatarAssetPath"));
	P.Charisma = static_cast<float>(Obj->GetNumberField(TEXT("charisma")));
	P.Intelligence = static_cast<float>(Obj->GetNumberField(TEXT("intelligence")));
	P.Physical = static_cast<float>(Obj->GetNumberField(TEXT("physical")));
	P.Loyalty = static_cast<float>(Obj->GetNumberField(TEXT("loyalty")));
	P.Deception = static_cast<float>(Obj->GetNumberField(TEXT("deception")));
	P.bEvicted = Obj->GetBoolField(TEXT("evicted"));
	P.EvictedOnDay = Obj->GetIntegerField(TEXT("evictedOnDay"));
	return P;
}

FBBRoom UBBSimDataSubsystem::ParseRoom(const TSharedPtr<FJsonObject>& Obj) const
{
	FBBRoom R;
	R.RoomId = Obj->GetStringField(TEXT("roomId"));
	R.RoomName = Obj->GetStringField(TEXT("roomName"));
	R.RoomType = StringToRoomType(Obj->GetStringField(TEXT("roomType")));
	R.Capacity = Obj->GetIntegerField(TEXT("capacity"));

	const TSharedPtr<FJsonObject>* PosObj;
	if (Obj->TryGetObjectField(TEXT("position"), PosObj))
	{
		R.Position.X = (*PosObj)->GetNumberField(TEXT("x"));
		R.Position.Y = (*PosObj)->GetNumberField(TEXT("y"));
		R.Position.Z = (*PosObj)->GetNumberField(TEXT("z"));
	}

	const TSharedPtr<FJsonObject>* ExtObj;
	if (Obj->TryGetObjectField(TEXT("extents"), ExtObj))
	{
		R.Extents.X = (*ExtObj)->GetNumberField(TEXT("x"));
		R.Extents.Y = (*ExtObj)->GetNumberField(TEXT("y"));
		R.Extents.Z = (*ExtObj)->GetNumberField(TEXT("z"));
	}

	return R;
}

FBBSnapshot UBBSimDataSubsystem::ParseSnapshot(const TSharedPtr<FJsonObject>& Obj) const
{
	FBBSnapshot S;
	S.Tick = Obj->GetIntegerField(TEXT("tick"));
	S.Day = Obj->GetIntegerField(TEXT("day"));
	S.Hour = Obj->GetIntegerField(TEXT("hour"));

	const TArray<TSharedPtr<FJsonValue>>* StatesArray;
	if (Obj->TryGetArrayField(TEXT("participantStates"), StatesArray))
	{
		for (const auto& Val : *StatesArray)
		{
			S.ParticipantStates.Add(ParseParticipantState(Val->AsObject()));
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* ActiveEventsArray;
	if (Obj->TryGetArrayField(TEXT("activeEvents"), ActiveEventsArray))
	{
		for (const auto& Val : *ActiveEventsArray)
		{
			S.ActiveEvents.Add(ParseGameEvent(Val->AsObject()));
		}
	}

	return S;
}

FBBParticipantState UBBSimDataSubsystem::ParseParticipantState(const TSharedPtr<FJsonObject>& Obj) const
{
	FBBParticipantState PS;
	PS.ParticipantId = Obj->GetStringField(TEXT("participantId"));
	PS.CurrentRoomId = Obj->GetStringField(TEXT("currentRoomId"));
	PS.Mood = StringToMood(Obj->GetStringField(TEXT("mood")));
	PS.MoodIntensity = static_cast<float>(Obj->GetNumberField(TEXT("moodIntensity")));
	PS.CurrentAction = Obj->GetStringField(TEXT("currentAction"));
	PS.StressLevel = static_cast<float>(Obj->GetNumberField(TEXT("stressLevel")));
	PS.bIsHeadOfHousehold = Obj->GetBoolField(TEXT("isHeadOfHousehold"));
	PS.bIsNominated = Obj->GetBoolField(TEXT("isNominated"));
	PS.bHasVeto = Obj->GetBoolField(TEXT("hasVeto"));

	const TSharedPtr<FJsonObject>* PosObj;
	if (Obj->TryGetObjectField(TEXT("worldPosition"), PosObj))
	{
		PS.WorldPosition.X = (*PosObj)->GetNumberField(TEXT("x"));
		PS.WorldPosition.Y = (*PosObj)->GetNumberField(TEXT("y"));
		PS.WorldPosition.Z = (*PosObj)->GetNumberField(TEXT("z"));
	}

	const TSharedPtr<FJsonObject>* RotObj;
	if (Obj->TryGetObjectField(TEXT("worldRotation"), RotObj))
	{
		PS.WorldRotation.Pitch = (*RotObj)->GetNumberField(TEXT("pitch"));
		PS.WorldRotation.Yaw = (*RotObj)->GetNumberField(TEXT("yaw"));
		PS.WorldRotation.Roll = (*RotObj)->GetNumberField(TEXT("roll"));
	}

	const TArray<TSharedPtr<FJsonValue>>* RelsArray;
	if (Obj->TryGetArrayField(TEXT("activeRelationships"), RelsArray))
	{
		for (const auto& Val : *RelsArray)
		{
			PS.ActiveRelationships.Add(Val->AsString());
		}
	}

	return PS;
}

FBBGameEvent UBBSimDataSubsystem::ParseGameEvent(const TSharedPtr<FJsonObject>& Obj) const
{
	FBBGameEvent E;
	E.EventId = Obj->GetStringField(TEXT("eventId"));
	E.Tick = Obj->GetIntegerField(TEXT("tick"));
	E.Day = Obj->GetIntegerField(TEXT("day"));
	E.Hour = Obj->GetIntegerField(TEXT("hour"));
	E.EventType = StringToEventType(Obj->GetStringField(TEXT("eventType")));
	E.RoomId = Obj->GetStringField(TEXT("roomId"));
	E.Description = Obj->GetStringField(TEXT("description"));
	E.DialogueText = Obj->GetStringField(TEXT("dialogueText"));
	E.AnimationHint = Obj->GetStringField(TEXT("animationHint"));
	E.DramaScore = static_cast<float>(Obj->GetNumberField(TEXT("dramaScore")));

	const TArray<TSharedPtr<FJsonValue>>* PidsArray;
	if (Obj->TryGetArrayField(TEXT("participantIds"), PidsArray))
	{
		for (const auto& Val : *PidsArray)
		{
			E.ParticipantIds.Add(Val->AsString());
		}
	}

	const TSharedPtr<FJsonObject>* MetaObj;
	if (Obj->TryGetObjectField(TEXT("metadata"), MetaObj))
	{
		for (const auto& Pair : (*MetaObj)->Values)
		{
			E.Metadata.Add(Pair.Key, Pair.Value->AsString());
		}
	}

	return E;
}

FBBDailyRecap UBBSimDataSubsystem::ParseDailyRecap(const TSharedPtr<FJsonObject>& Obj) const
{
	FBBDailyRecap R;
	R.Day = Obj->GetIntegerField(TEXT("day"));
	R.Summary = Obj->GetStringField(TEXT("summary"));
	R.HeadOfHouseholdId = Obj->GetStringField(TEXT("headOfHouseholdId"));
	R.EvictedId = Obj->GetStringField(TEXT("evictedId"));
	R.DramaRating = static_cast<float>(Obj->GetNumberField(TEXT("dramaRating")));

	auto ParseStringArray = [](const TSharedPtr<FJsonObject>& InObj, const FString& FieldName, TArray<FString>& OutArray)
	{
		const TArray<TSharedPtr<FJsonValue>>* Arr;
		if (InObj->TryGetArrayField(FieldName, Arr))
		{
			for (const auto& Val : *Arr)
			{
				OutArray.Add(Val->AsString());
			}
		}
	};

	ParseStringArray(Obj, TEXT("keyEventIds"), R.KeyEventIds);
	ParseStringArray(Obj, TEXT("alliancesFormed"), R.AlliancesFormed);
	ParseStringArray(Obj, TEXT("conflictsOccurred"), R.ConflictsOccurred);
	ParseStringArray(Obj, TEXT("nomineeIds"), R.NomineeIds);

	return R;
}

FBBHighlight UBBSimDataSubsystem::ParseHighlight(const TSharedPtr<FJsonObject>& Obj) const
{
	FBBHighlight H;
	H.HighlightId = Obj->GetStringField(TEXT("highlightId"));
	H.Title = Obj->GetStringField(TEXT("title"));
	H.Description = Obj->GetStringField(TEXT("description"));
	H.StartTick = Obj->GetIntegerField(TEXT("startTick"));
	H.EndTick = Obj->GetIntegerField(TEXT("endTick"));
	H.PrimaryEventType = StringToEventType(Obj->GetStringField(TEXT("primaryEventType")));
	H.DramaScore = static_cast<float>(Obj->GetNumberField(TEXT("dramaScore")));

	const TArray<TSharedPtr<FJsonValue>>* PidsArray;
	if (Obj->TryGetArrayField(TEXT("participantIds"), PidsArray))
	{
		for (const auto& Val : *PidsArray)
		{
			H.ParticipantIds.Add(Val->AsString());
		}
	}

	return H;
}

FBBSnapshot UBBSimDataSubsystem::GetSnapshotAtTick(int32 Tick) const
{
	if (!bSeasonLoaded || CachedSeason.Snapshots.Num() == 0)
	{
		return FBBSnapshot();
	}

	// Find exact match first
	if (const int32* IndexPtr = TickToSnapshotIndex.Find(Tick))
	{
		return CachedSeason.Snapshots[*IndexPtr];
	}

	// Binary search for nearest snapshot at or before Tick
	int32 BestIndex = 0;
	for (int32 i = 0; i < CachedSeason.Snapshots.Num(); ++i)
	{
		if (CachedSeason.Snapshots[i].Tick <= Tick)
		{
			BestIndex = i;
		}
		else
		{
			break;
		}
	}
	return CachedSeason.Snapshots[BestIndex];
}

TArray<FBBGameEvent> UBBSimDataSubsystem::GetEventsAtTick(int32 Tick) const
{
	TArray<FBBGameEvent> Result;
	if (const TArray<int32>* Indices = TickToEventIndices.Find(Tick))
	{
		for (int32 Idx : *Indices)
		{
			if (CachedSeason.Events.IsValidIndex(Idx))
			{
				Result.Add(CachedSeason.Events[Idx]);
			}
		}
	}
	return Result;
}

TArray<FBBGameEvent> UBBSimDataSubsystem::GetEventsInRange(int32 StartTick, int32 EndTick) const
{
	TArray<FBBGameEvent> Result;
	for (const FBBGameEvent& Evt : CachedSeason.Events)
	{
		if (Evt.Tick >= StartTick && Evt.Tick <= EndTick)
		{
			Result.Add(Evt);
		}
	}
	return Result;
}

FBBParticipantState UBBSimDataSubsystem::GetParticipantStateAtTick(const FString& ParticipantId, int32 Tick) const
{
	FBBSnapshot Snap = GetSnapshotAtTick(Tick);
	for (const FBBParticipantState& PS : Snap.ParticipantStates)
	{
		if (PS.ParticipantId == ParticipantId)
		{
			return PS;
		}
	}
	return FBBParticipantState();
}

FBBParticipant UBBSimDataSubsystem::GetParticipant(const FString& ParticipantId) const
{
	for (const FBBParticipant& P : CachedSeason.Participants)
	{
		if (P.ParticipantId == ParticipantId)
		{
			return P;
		}
	}
	return FBBParticipant();
}

FBBDailyRecap UBBSimDataSubsystem::GetDailyRecap(int32 Day) const
{
	for (const FBBDailyRecap& R : CachedSeason.DailyRecaps)
	{
		if (R.Day == Day)
		{
			return R;
		}
	}
	return FBBDailyRecap();
}

TArray<FBBHighlight> UBBSimDataSubsystem::GetHighlightsForParticipant(const FString& ParticipantId) const
{
	TArray<FBBHighlight> Result;
	for (const FBBHighlight& H : CachedSeason.Highlights)
	{
		if (H.ParticipantIds.Contains(ParticipantId))
		{
			Result.Add(H);
		}
	}
	return Result;
}

FBBRoom UBBSimDataSubsystem::GetRoom(const FString& RoomId) const
{
	for (const FBBRoom& R : CachedSeason.Rooms)
	{
		if (R.RoomId == RoomId)
		{
			return R;
		}
	}
	return FBBRoom();
}

FBBSnapshot UBBSimDataSubsystem::ScrubToTick(int32 Tick) const
{
	if (!bSeasonLoaded || CachedSeason.Snapshots.Num() == 0)
	{
		return FBBSnapshot();
	}

	// Clamp tick
	Tick = FMath::Clamp(Tick, 0, CachedSeason.TotalTicks);

	// Find surrounding snapshots for interpolation
	int32 BeforeIdx = 0;
	int32 AfterIdx = 0;
	for (int32 i = 0; i < CachedSeason.Snapshots.Num(); ++i)
	{
		if (CachedSeason.Snapshots[i].Tick <= Tick)
		{
			BeforeIdx = i;
		}
		if (CachedSeason.Snapshots[i].Tick >= Tick)
		{
			AfterIdx = i;
			break;
		}
		AfterIdx = i;
	}

	// Exact match or only one snapshot available
	if (BeforeIdx == AfterIdx || CachedSeason.Snapshots[BeforeIdx].Tick == Tick)
	{
		return CachedSeason.Snapshots[BeforeIdx];
	}

	// Interpolate between two snapshots
	const FBBSnapshot& Before = CachedSeason.Snapshots[BeforeIdx];
	const FBBSnapshot& After = CachedSeason.Snapshots[AfterIdx];

	float Alpha = 0.f;
	int32 TickDelta = After.Tick - Before.Tick;
	if (TickDelta > 0)
	{
		Alpha = static_cast<float>(Tick - Before.Tick) / static_cast<float>(TickDelta);
	}

	FBBSnapshot Interpolated;
	Interpolated.Tick = Tick;
	Interpolated.Day = Before.Day;
	Interpolated.Hour = Before.Hour;
	Interpolated.ActiveEvents = Before.ActiveEvents;

	// Interpolate participant positions
	for (int32 i = 0; i < Before.ParticipantStates.Num(); ++i)
	{
		FBBParticipantState InterpState = Before.ParticipantStates[i];

		// Find matching state in After snapshot
		for (const FBBParticipantState& AfterState : After.ParticipantStates)
		{
			if (AfterState.ParticipantId == InterpState.ParticipantId)
			{
				InterpState.WorldPosition = FMath::Lerp(InterpState.WorldPosition, AfterState.WorldPosition, Alpha);
				InterpState.WorldRotation = FMath::Lerp(InterpState.WorldRotation, AfterState.WorldRotation, Alpha);
				InterpState.MoodIntensity = FMath::Lerp(InterpState.MoodIntensity, AfterState.MoodIntensity, Alpha);
				InterpState.StressLevel = FMath::Lerp(InterpState.StressLevel, AfterState.StressLevel, Alpha);

				// Snap discrete states from the later snapshot if we're past halfway
				if (Alpha > 0.5f)
				{
					InterpState.Mood = AfterState.Mood;
					InterpState.CurrentRoomId = AfterState.CurrentRoomId;
					InterpState.CurrentAction = AfterState.CurrentAction;
					InterpState.bIsHeadOfHousehold = AfterState.bIsHeadOfHousehold;
					InterpState.bIsNominated = AfterState.bIsNominated;
					InterpState.bHasVeto = AfterState.bHasVeto;
				}
				break;
			}
		}

		Interpolated.ParticipantStates.Add(InterpState);
	}

	return Interpolated;
}

EBBEventType UBBSimDataSubsystem::StringToEventType(const FString& Str) const
{
	if (Str.Equals(TEXT("Alliance"), ESearchCase::IgnoreCase)) return EBBEventType::Alliance;
	if (Str.Equals(TEXT("Conflict"), ESearchCase::IgnoreCase)) return EBBEventType::Conflict;
	if (Str.Equals(TEXT("Betrayal"), ESearchCase::IgnoreCase)) return EBBEventType::Betrayal;
	if (Str.Equals(TEXT("Nomination"), ESearchCase::IgnoreCase)) return EBBEventType::Nomination;
	if (Str.Equals(TEXT("Eviction"), ESearchCase::IgnoreCase)) return EBBEventType::Eviction;
	if (Str.Equals(TEXT("Confession"), ESearchCase::IgnoreCase)) return EBBEventType::Confession;
	if (Str.Equals(TEXT("Competition"), ESearchCase::IgnoreCase)) return EBBEventType::Competition;
	if (Str.Equals(TEXT("SocialChat"), ESearchCase::IgnoreCase)) return EBBEventType::SocialChat;
	if (Str.Equals(TEXT("Romance"), ESearchCase::IgnoreCase)) return EBBEventType::Romance;
	if (Str.Equals(TEXT("Strategy"), ESearchCase::IgnoreCase)) return EBBEventType::Strategy;
	return EBBEventType::None;
}

EBBMood UBBSimDataSubsystem::StringToMood(const FString& Str) const
{
	if (Str.Equals(TEXT("Happy"), ESearchCase::IgnoreCase)) return EBBMood::Happy;
	if (Str.Equals(TEXT("Sad"), ESearchCase::IgnoreCase)) return EBBMood::Sad;
	if (Str.Equals(TEXT("Angry"), ESearchCase::IgnoreCase)) return EBBMood::Angry;
	if (Str.Equals(TEXT("Anxious"), ESearchCase::IgnoreCase)) return EBBMood::Anxious;
	if (Str.Equals(TEXT("Excited"), ESearchCase::IgnoreCase)) return EBBMood::Excited;
	if (Str.Equals(TEXT("Scheming"), ESearchCase::IgnoreCase)) return EBBMood::Scheming;
	if (Str.Equals(TEXT("Betrayed"), ESearchCase::IgnoreCase)) return EBBMood::Betrayed;
	return EBBMood::Neutral;
}

EBBRoomType UBBSimDataSubsystem::StringToRoomType(const FString& Str) const
{
	if (Str.Equals(TEXT("LivingRoom"), ESearchCase::IgnoreCase)) return EBBRoomType::LivingRoom;
	if (Str.Equals(TEXT("Kitchen"), ESearchCase::IgnoreCase)) return EBBRoomType::Kitchen;
	if (Str.Equals(TEXT("Bedroom"), ESearchCase::IgnoreCase)) return EBBRoomType::Bedroom;
	if (Str.Equals(TEXT("Bathroom"), ESearchCase::IgnoreCase)) return EBBRoomType::Bathroom;
	if (Str.Equals(TEXT("Garden"), ESearchCase::IgnoreCase)) return EBBRoomType::Garden;
	if (Str.Equals(TEXT("Confessional"), ESearchCase::IgnoreCase)) return EBBRoomType::Confessional;
	if (Str.Equals(TEXT("CompetitionArea"), ESearchCase::IgnoreCase)) return EBBRoomType::CompetitionArea;
	if (Str.Equals(TEXT("HoHRoom"), ESearchCase::IgnoreCase)) return EBBRoomType::HoHRoom;
	if (Str.Equals(TEXT("DiningRoom"), ESearchCase::IgnoreCase)) return EBBRoomType::DiningRoom;
	if (Str.Equals(TEXT("Pool"), ESearchCase::IgnoreCase)) return EBBRoomType::Pool;
	return EBBRoomType::LivingRoom;
}
