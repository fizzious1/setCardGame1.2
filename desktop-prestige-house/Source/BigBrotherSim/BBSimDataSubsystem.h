// Copyright Big Brother AI Simulator. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BBSimDataSubsystem.generated.h"

// ─── Enums ──────────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EBBEventType : uint8
{
	None				UMETA(DisplayName = "None"),
	Alliance			UMETA(DisplayName = "Alliance"),
	Conflict			UMETA(DisplayName = "Conflict"),
	Betrayal			UMETA(DisplayName = "Betrayal"),
	Nomination			UMETA(DisplayName = "Nomination"),
	Eviction			UMETA(DisplayName = "Eviction"),
	Confession			UMETA(DisplayName = "Confession"),
	Competition			UMETA(DisplayName = "Competition"),
	SocialChat			UMETA(DisplayName = "Social Chat"),
	Romance				UMETA(DisplayName = "Romance"),
	Strategy			UMETA(DisplayName = "Strategy")
};

UENUM(BlueprintType)
enum class EBBMood : uint8
{
	Happy		UMETA(DisplayName = "Happy"),
	Sad			UMETA(DisplayName = "Sad"),
	Angry		UMETA(DisplayName = "Angry"),
	Anxious		UMETA(DisplayName = "Anxious"),
	Excited		UMETA(DisplayName = "Excited"),
	Neutral		UMETA(DisplayName = "Neutral"),
	Scheming	UMETA(DisplayName = "Scheming"),
	Betrayed	UMETA(DisplayName = "Betrayed")
};

UENUM(BlueprintType)
enum class EBBRoomType : uint8
{
	LivingRoom		UMETA(DisplayName = "Living Room"),
	Kitchen			UMETA(DisplayName = "Kitchen"),
	Bedroom			UMETA(DisplayName = "Bedroom"),
	Bathroom		UMETA(DisplayName = "Bathroom"),
	Garden			UMETA(DisplayName = "Garden"),
	Confessional	UMETA(DisplayName = "Confessional"),
	CompetitionArea	UMETA(DisplayName = "Competition Area"),
	HoHRoom			UMETA(DisplayName = "Head of Household Room"),
	DiningRoom		UMETA(DisplayName = "Dining Room"),
	Pool			UMETA(DisplayName = "Pool")
};

// ─── Data Structs ───────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FBBRoom
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RoomId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RoomName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EBBRoomType RoomType = EBBRoomType::LivingRoom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Capacity = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Position = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Extents = FVector(500.f, 500.f, 300.f);
};

USTRUCT(BlueprintType)
struct FBBParticipant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ParticipantId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Age = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Occupation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Bio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString AvatarAssetPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Charisma = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Intelligence = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Physical = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Loyalty = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Deception = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEvicted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EvictedOnDay = -1;
};

USTRUCT(BlueprintType)
struct FBBParticipantState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ParticipantId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CurrentRoomId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EBBMood Mood = EBBMood::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoodIntensity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector WorldPosition = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator WorldRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CurrentAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> ActiveRelationships;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StressLevel = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsHeadOfHousehold = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsNominated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasVeto = false;
};

USTRUCT(BlueprintType)
struct FBBGameEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString EventId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Tick = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Day = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Hour = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EBBEventType EventType = EBBEventType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> ParticipantIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RoomId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DialogueText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString AnimationHint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DramaScore = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, FString> Metadata;
};

USTRUCT(BlueprintType)
struct FBBSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Tick = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Day = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Hour = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBBParticipantState> ParticipantStates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBBGameEvent> ActiveEvents;
};

USTRUCT(BlueprintType)
struct FBBDailyRecap
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Day = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Summary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> KeyEventIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> AlliancesFormed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> ConflictsOccurred;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString HeadOfHouseholdId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> NomineeIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString EvictedId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DramaRating = 0.f;
};

USTRUCT(BlueprintType)
struct FBBHighlight
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString HighlightId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 StartTick = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 EndTick = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> ParticipantIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EBBEventType PrimaryEventType = EBBEventType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DramaScore = 0.f;
};

USTRUCT(BlueprintType)
struct FBBSeason
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SeasonId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SeasonName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TotalDays = 70;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TotalTicks = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBBParticipant> Participants;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBBRoom> Rooms;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBBSnapshot> Snapshots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBBGameEvent> Events;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBBDailyRecap> DailyRecaps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBBHighlight> Highlights;
};

// ─── Subsystem ──────────────────────────────────────────────────────────────

UCLASS()
class BIGBROTHERSIM_API UBBSimDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Load season data from a JSON file path */
	UFUNCTION(BlueprintCallable, Category = "BB|Data")
	bool LoadSeasonFromFile(const FString& FilePath);

	/** Load season data from a JSON string */
	UFUNCTION(BlueprintCallable, Category = "BB|Data")
	bool LoadSeasonFromString(const FString& JsonString);

	/** Get the loaded season data */
	UFUNCTION(BlueprintCallable, Category = "BB|Data")
	const FBBSeason& GetSeason() const { return CachedSeason; }

	/** Check if season data is loaded */
	UFUNCTION(BlueprintPure, Category = "BB|Data")
	bool IsSeasonLoaded() const { return bSeasonLoaded; }

	/** Get snapshot at or before a given tick */
	UFUNCTION(BlueprintCallable, Category = "BB|Data")
	FBBSnapshot GetSnapshotAtTick(int32 Tick) const;

	/** Get all events at a specific tick */
	UFUNCTION(BlueprintCallable, Category = "BB|Data")
	TArray<FBBGameEvent> GetEventsAtTick(int32 Tick) const;

	/** Get events within a tick range */
	UFUNCTION(BlueprintCallable, Category = "BB|Data")
	TArray<FBBGameEvent> GetEventsInRange(int32 StartTick, int32 EndTick) const;

	/** Get participant state at a tick */
	UFUNCTION(BlueprintCallable, Category = "BB|Data")
	FBBParticipantState GetParticipantStateAtTick(const FString& ParticipantId, int32 Tick) const;

	/** Get participant base info by ID */
	UFUNCTION(BlueprintCallable, Category = "BB|Data")
	FBBParticipant GetParticipant(const FString& ParticipantId) const;

	/** Get daily recap for a specific day */
	UFUNCTION(BlueprintCallable, Category = "BB|Data")
	FBBDailyRecap GetDailyRecap(int32 Day) const;

	/** Get all highlights */
	UFUNCTION(BlueprintCallable, Category = "BB|Data")
	TArray<FBBHighlight> GetHighlights() const { return CachedSeason.Highlights; }

	/** Get highlights for a specific participant */
	UFUNCTION(BlueprintCallable, Category = "BB|Data")
	TArray<FBBHighlight> GetHighlightsForParticipant(const FString& ParticipantId) const;

	/** Get room data by ID */
	UFUNCTION(BlueprintCallable, Category = "BB|Data")
	FBBRoom GetRoom(const FString& RoomId) const;

	/** Get all rooms */
	UFUNCTION(BlueprintCallable, Category = "BB|Data")
	TArray<FBBRoom> GetAllRooms() const { return CachedSeason.Rooms; }

	/** Get total tick count */
	UFUNCTION(BlueprintPure, Category = "BB|Data")
	int32 GetTotalTicks() const { return CachedSeason.TotalTicks; }

	/** Scrub to a specific tick: returns interpolated snapshot */
	UFUNCTION(BlueprintCallable, Category = "BB|Data")
	FBBSnapshot ScrubToTick(int32 Tick) const;

private:
	bool ParseSeasonJson(TSharedPtr<FJsonObject> JsonObject);
	FBBParticipant ParseParticipant(const TSharedPtr<FJsonObject>& Obj) const;
	FBBRoom ParseRoom(const TSharedPtr<FJsonObject>& Obj) const;
	FBBSnapshot ParseSnapshot(const TSharedPtr<FJsonObject>& Obj) const;
	FBBParticipantState ParseParticipantState(const TSharedPtr<FJsonObject>& Obj) const;
	FBBGameEvent ParseGameEvent(const TSharedPtr<FJsonObject>& Obj) const;
	FBBDailyRecap ParseDailyRecap(const TSharedPtr<FJsonObject>& Obj) const;
	FBBHighlight ParseHighlight(const TSharedPtr<FJsonObject>& Obj) const;

	EBBEventType StringToEventType(const FString& Str) const;
	EBBMood StringToMood(const FString& Str) const;
	EBBRoomType StringToRoomType(const FString& Str) const;

	FBBSeason CachedSeason;
	bool bSeasonLoaded = false;

	/** Tick-indexed caches for fast lookup */
	TMap<int32, int32> TickToSnapshotIndex;
	TMap<int32, TArray<int32>> TickToEventIndices;
};
