// Impact Forge LLC 2024


#include "ScenarioPersistenceManager.h"

#include "JsonObjectConverter.h"
#include "GameFramework/GameStateBase.h"

void UScenarioPersistenceManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadPersistedData();
}

void UScenarioPersistenceManager::Deinitialize()
{
	SavePersistedData();
	Super::Deinitialize();
}

void UScenarioPersistenceManager::SaveScenarioStats(const FScenarioStats& Stats)
{
	ScenarioStatistics.Add(Stats.ScenarioId, Stats);
	SavePersistedData();
}

FScenarioStats UScenarioPersistenceManager::GetScenarioStats(const FPrimaryAssetId& ScenarioId) const
{
	if (const FScenarioStats* Stats = ScenarioStatistics.Find(ScenarioId))
	{
		return *Stats;
	}
	return FScenarioStats();
}

void UScenarioPersistenceManager::UpdatePlayCount(const FPrimaryAssetId& ScenarioId)
{
	FScenarioStats& Stats = ScenarioStatistics.FindOrAdd(ScenarioId);
	Stats.ScenarioId = ScenarioId;
	Stats.TimesPlayed++;
	Stats.LastPlayed = FDateTime::UtcNow();

	// Update average player count if we have a valid world
	if (UWorld* World = GetWorld())
	{
		if (AGameStateBase* GameState = World->GetGameState())
		{
			float CurrentPlayerCount = GameState->PlayerArray.Num();
			Stats.AveragePlayerCount = ((Stats.AveragePlayerCount * (Stats.TimesPlayed - 1)) + CurrentPlayerCount) / Stats.TimesPlayed;
		}
	}

	SavePersistedData();
}

void UScenarioPersistenceManager::SetRotationEntry(const FScenarioRotationEntry& Entry)
{
	// Remove existing entry if present
	RotationEntries.RemoveAll([&Entry](const FScenarioRotationEntry& ExistingEntry) {
		return ExistingEntry.ScenarioId == Entry.ScenarioId;
	});

	RotationEntries.Add(Entry);
	SavePersistedData();
}

TArray<FPrimaryAssetId> UScenarioPersistenceManager::GetNextRotationOptions() const
{
	TArray<FPrimaryAssetId> ValidOptions;
	const FDateTime CurrentTime = FDateTime::UtcNow();

	// Filter scenarios based on minimum gap and weight
	for (const FScenarioRotationEntry& Entry : RotationEntries)
	{
		if (const FScenarioStats* Stats = ScenarioStatistics.Find(Entry.ScenarioId))
		{
			// Check if enough time has passed since last play
			FTimespan TimeSinceLastPlay = CurrentTime - Stats->LastPlayed;
			if (TimeSinceLastPlay.GetDays() >= Entry.MinimumGapBetweenPlays)
			{
				// Add scenario multiple times based on weight to influence random selection
				int32 WeightedCount = FMath::RoundToInt(Entry.Weight * 10.0f);
				for (int32 i = 0; i < WeightedCount; ++i)
				{
					ValidOptions.Add(Entry.ScenarioId);
				}
			}
		}
		else
		{
			// If never played, always include
			ValidOptions.Add(Entry.ScenarioId);
		}
	}

	return ValidOptions;
}

bool UScenarioPersistenceManager::IsScenarioAllowedInRotation(const FPrimaryAssetId& ScenarioId) const
{
	const FDateTime CurrentTime = FDateTime::UtcNow();

	// Check if scenario is in rotation and meets time requirements
	for (const FScenarioRotationEntry& Entry : RotationEntries)
	{
		if (Entry.ScenarioId == ScenarioId)
		{
			if (const FScenarioStats* Stats = ScenarioStatistics.Find(ScenarioId))
			{
				FTimespan TimeSinceLastPlay = CurrentTime - Stats->LastPlayed;
				return TimeSinceLastPlay.GetDays() >= Entry.MinimumGapBetweenPlays;
			}
			return true; // Never played before, so it's allowed
		}
	}

	return false; // Not in rotation
}

TArray<FPrimaryAssetId> UScenarioPersistenceManager::GetWeightedScenarioOptions(int32 Count) const
{
	TArray<FPrimaryAssetId> WeightedOptions;
	TArray<TPair<FPrimaryAssetId, float>> ScoredScenarios;

	// Calculate scores for each scenario
	for (const auto& StatPair : ScenarioStatistics)
	{
		const FScenarioStats& Stats = StatPair.Value;
		float PopularityScore = 0.0f;

		if (Stats.TimesPlayed > 0)
		{
			// Calculate score based on votes per play and average player count
			float VotesPerPlay = static_cast<float>(Stats.TotalVotes) / Stats.TimesPlayed;
			PopularityScore = (VotesPerPlay * 0.7f) + (Stats.AveragePlayerCount * 0.3f);
		}

		ScoredScenarios.Add(TPair<FPrimaryAssetId, float>(Stats.ScenarioId, PopularityScore));
	}

	// Sort by score
	ScoredScenarios.Sort([](const TPair<FPrimaryAssetId, float>& A, const TPair<FPrimaryAssetId, float>& B) {
		return A.Value > B.Value;
	});

	// Get top Count scenarios
	for (int32 i = 0; i < FMath::Min(Count, ScoredScenarios.Num()); ++i)
	{
		WeightedOptions.Add(ScoredScenarios[i].Key);
	}

	return WeightedOptions;
}

void UScenarioPersistenceManager::LoadPersistedData()
{
	FString SaveFilePath = GetSaveFilePath();
	FString JsonString;

	if (FFileHelper::LoadFileToString(JsonString, *SaveFilePath))
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			// Load scenario statistics
			const TArray<TSharedPtr<FJsonValue>>* StatsArray;
			if (JsonObject->TryGetArrayField(TEXT("ScenarioStats"), StatsArray))
			{
				for (const auto& JsonValue : *StatsArray)
				{
					FScenarioStats Stats;
					if (FJsonObjectConverter::JsonObjectToUStruct(JsonValue->AsObject().ToSharedRef(), &Stats))
					{
						ScenarioStatistics.Add(Stats.ScenarioId, Stats);
					}
				}
			}

			// Load rotation entries
			const TArray<TSharedPtr<FJsonValue>>* RotationArray;
			if (JsonObject->TryGetArrayField(TEXT("RotationEntries"), RotationArray))
			{
				for (const auto& JsonValue : *RotationArray)
				{
					FScenarioRotationEntry Entry;
					if (FJsonObjectConverter::JsonObjectToUStruct(JsonValue->AsObject().ToSharedRef(), &Entry))
					{
						RotationEntries.Add(Entry);
					}
				}
			}
		}
	}
}

void UScenarioPersistenceManager::SavePersistedData()
{
	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    
	// Save scenario statistics
	TArray<TSharedPtr<FJsonValue>> StatsArray;
	for (const auto& StatPair : ScenarioStatistics)
	{
		TSharedPtr<FJsonObject> StatObject = MakeShared<FJsonObject>();
		if (FJsonObjectConverter::UStructToJsonObject(FScenarioStats::StaticStruct(), &StatPair.Value, StatObject.ToSharedRef(), 0, 0))
		{
			StatsArray.Add(MakeShared<FJsonValueObject>(StatObject));
		}
	}
	JsonObject->SetArrayField(TEXT("ScenarioStats"), StatsArray);

	// Save rotation entries
	TArray<TSharedPtr<FJsonValue>> RotationArray;
	for (const auto& Entry : RotationEntries)
	{
		TSharedPtr<FJsonObject> EntryObject = MakeShared<FJsonObject>();
		if (FJsonObjectConverter::UStructToJsonObject(FScenarioRotationEntry::StaticStruct(), &Entry, EntryObject.ToSharedRef(), 0, 0))
		{
			RotationArray.Add(MakeShared<FJsonValueObject>(EntryObject));
		}
	}
	JsonObject->SetArrayField(TEXT("RotationEntries"), RotationArray);

	// Save to file
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		FFileHelper::SaveStringToFile(JsonString, *GetSaveFilePath());
	}
}

FString UScenarioPersistenceManager::GetSaveFilePath() const
{
	return FPaths::ProjectSavedDir() / TEXT("ScenarioStats.json");
}
