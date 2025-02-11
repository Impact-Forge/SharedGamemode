// Impact Forge LLC 2024

#pragma once

#include "CoreMinimal.h"
#include "ScenarioTransitionComponent.h"
#include "AbilitySystem/Phases/BSGamePhaseSubsystem.h"
#include "Components/GameFrameworkComponent.h"
#include "EnhancedScenarioTransitionComponent.generated.h"

class AGameModeBase;

USTRUCT(BlueprintType)
struct FPlayerVoteWeight
{
	GENERATED_BODY()

	UPROPERTY()
	float BaseWeight;

	UPROPERTY()
	float PerformanceMultiplier;

	UPROPERTY()
	bool bHasVetoed;

	FPlayerVoteWeight()
		: BaseWeight(1.0f)
		, PerformanceMultiplier(1.0f)
		, bHasVetoed(false)
	{}
};

USTRUCT(BlueprintType)
struct FEnhancedVoteEntry : public FScenarioVoteEntry
{
	GENERATED_BODY()

	UPROPERTY()
	float WeightedVotes;

	UPROPERTY()
	int32 VetoCount;

	FEnhancedVoteEntry()
		: WeightedVotes(0.0f)
		, VetoCount(0)
	{}
};

USTRUCT(BlueprintType)
struct FEnhancedVotingState : public FScenarioVotingState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FEnhancedVoteEntry> EnhancedVoteOptions;

	UPROPERTY()
	bool bAllowVetos;

	UPROPERTY()
	int32 VetoThreshold;

	FEnhancedVotingState()
		: bAllowVetos(true)
		, VetoThreshold(3)
	{}
};

UCLASS()
class SHAREDGAMEMODE_API UEnhancedScenarioTransitionComponent : public UScenarioTransitionComponent
{
	GENERATED_BODY()

public:
	UEnhancedScenarioTransitionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Configuration
	UPROPERTY(EditDefaultsOnly, Category = "Voting")
	float MinimumVoteWeight;

	UPROPERTY(EditDefaultsOnly, Category = "Voting")
	float MaximumVoteWeight;

	UPROPERTY(EditDefaultsOnly, Category = "Voting")
	float PerformanceWeightMultiplier;

	// Enhanced voting functions
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Voting")
	void ServerVetoScenario(const FPrimaryAssetId& ScenarioId);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Voting")
	void ServerUpdatePlayerPerformance(APlayerState* PlayerState, float PerformanceScore);

protected:
	virtual void GenerateVotingOptions() override;
	virtual void ProcessVotingResults() override;

	// Track player performance and voting weights
	UPROPERTY()
	TMap<APlayerState*, FPlayerVoteWeight> PlayerWeights;

	// Map to store player votes
	UPROPERTY()
	TMap<APlayerState*, FPrimaryAssetId> PlayerVotes;
	
	// Calculate weighted votes
	float CalculatePlayerVoteWeight(APlayerState* PlayerState) const;
	void UpdateVoteWeights();

private:
	// Helper functions
	void InitializePlayerWeight(APlayerState* PlayerState);
	bool IsScenarioVetoed(const FPrimaryAssetId& ScenarioId) const;
	void ApplyRotationRules();
};

/**
 * Component to handle scenario-related functionality in any GameMode
 */
UCLASS()
class SHAREDGAMEMODE_API UScenarioGameModeComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()


public:
	UScenarioGameModeComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

	// Performance tracking functions
	UFUNCTION(BlueprintCallable, Category = "Scenario")
	void UpdatePlayerPerformance(APlayerState* PlayerState, float Score);

	// Game flow functions
	UFUNCTION(BlueprintCallable, Category = "Scenario")
	void StartScenarioVoting();

	// Phase to trigger voting
	UPROPERTY(EditDefaultsOnly, Category = "Scenario")
	FGameplayTag EndGamePhaseTag;

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	UFUNCTION()
	void HandlePhaseChange(const FGameplayTag& PhaseTag);

private:
	// Weak pointer to owning GameMode
	TWeakObjectPtr<AGameModeBase> OwningGameMode;

	// Reference to phase subsystem
	UPROPERTY()
	UBSGamePhaseSubsystem* PhaseSubsystem;
};
