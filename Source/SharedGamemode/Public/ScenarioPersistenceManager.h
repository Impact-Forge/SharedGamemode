// Impact Forge LLC 2024

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ScenarioPersistenceManager.generated.h"

USTRUCT()
struct FScenarioStats
{
	GENERATED_BODY()

	UPROPERTY()
	FPrimaryAssetId ScenarioId;

	UPROPERTY()
	int32 TimesPlayed;

	UPROPERTY()
	int32 TotalVotes;

	UPROPERTY()
	float AveragePlayerCount;

	UPROPERTY()
	FDateTime LastPlayed;

	FScenarioStats()
		: TimesPlayed(0)
		, TotalVotes(0)
		, AveragePlayerCount(0.0f)
	{}
};

USTRUCT()
struct FScenarioRotationEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FPrimaryAssetId ScenarioId;

	UPROPERTY()
	float Weight;

	UPROPERTY()
	int32 MinimumGapBetweenPlays;

	FScenarioRotationEntry()
		: Weight(1.0f)
		, MinimumGapBetweenPlays(1)
	{}
};

/**
 * 
 */
UCLASS()
class SHAREDGAMEMODE_API UScenarioPersistenceManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Persistence functions
	void SaveScenarioStats(const FScenarioStats& Stats);
	FScenarioStats GetScenarioStats(const FPrimaryAssetId& ScenarioId) const;
	void UpdatePlayCount(const FPrimaryAssetId& ScenarioId);

	// Rotation management
	void SetRotationEntry(const FScenarioRotationEntry& Entry);
	TArray<FPrimaryAssetId> GetNextRotationOptions() const;
	bool IsScenarioAllowedInRotation(const FPrimaryAssetId& ScenarioId) const;

	// Get weighted scenarios based on popularity
	TArray<FPrimaryAssetId> GetWeightedScenarioOptions(int32 Count) const;

private:
	// Persistent data
	UPROPERTY()
	TMap<FPrimaryAssetId, FScenarioStats> ScenarioStatistics;

	UPROPERTY()
	TArray<FScenarioRotationEntry> RotationEntries;

	// File operations
	void LoadPersistedData();
	void SavePersistedData();

	FString GetSaveFilePath() const;
};
