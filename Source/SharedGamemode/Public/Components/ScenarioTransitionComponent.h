// Impact Forge LLC 2024

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/GameStateComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ScenarioTransitionComponent.generated.h"

// Represents a single vote entry
USTRUCT(BlueprintType)
struct FScenarioVoteEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FPrimaryAssetId ScenarioId;

	UPROPERTY()
	int32 VoteCount;

	FScenarioVoteEntry()
		: VoteCount(0)
	{}

	void PreReplicatedRemove(const struct FScenarioVotingState& InArraySerializer);
	void PostReplicatedAdd(const struct FScenarioVotingState& InArraySerializer);
	void PostReplicatedChange(const struct FScenarioVotingState& InArraySerializer);
};

// Represents the current voting state
USTRUCT(BlueprintType)
struct FScenarioVotingState : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	UScenarioTransitionComponent* ScenarioComp;
	
	// Available scenarios to vote on
	UPROPERTY()
	TArray<FScenarioVoteEntry> VoteOptions;

	// Time remaining for voting
	UPROPERTY()
	float VoteTimeRemaining;

	// Is voting active?
	UPROPERTY()
	bool bVotingActive;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FScenarioVoteEntry, FScenarioVotingState>(VoteOptions, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FScenarioVotingState> : public TStructOpsTypeTraitsBase2<FScenarioVotingState>
{
	enum 
	{
		WithNetDeltaSerializer = true
	};
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVotingStateChanged, const FScenarioVotingState&, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNextScenarioSelected, FPrimaryAssetId, SelectedScenario);


UCLASS()
class SHAREDGAMEMODE_API UScenarioTransitionComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UScenarioTransitionComponent(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Configuration
	UPROPERTY(EditDefaultsOnly, Category = "Voting")
	float VotingDuration;

	UPROPERTY(EditDefaultsOnly, Category = "Voting")
	int32 NumScenarioOptions;

	UPROPERTY(EditDefaultsOnly, Category = "Voting")
	FGameplayTagQuery ScenarioFilter;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Voting")
	FOnVotingStateChanged OnVotingStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Voting")
	FOnNextScenarioSelected OnNextScenarioSelected;

	// Functions
	UFUNCTION(BlueprintCallable, Category = "Voting")
	void StartVoting();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Voting")
	void ServerCastVote(const FPrimaryAssetId& ScenarioId);

	UFUNCTION(BlueprintPure, Category = "Voting")
	const FScenarioVotingState& GetCurrentVotingState() const { return VotingState; }

	UFUNCTION()
	void OnRep_VotingState();
	
protected:
	// Replicated state
	UPROPERTY(ReplicatedUsing = OnRep_VotingState)
	FScenarioVotingState VotingState;



	// Internal functions
	virtual void GenerateVotingOptions();
	virtual void ProcessVotingResults();
    
	// Timer handle for voting period
	FTimerHandle VotingTimerHandle;

private:

	
	// Helper to get valid scenarios
	TArray<FPrimaryAssetId> GetValidScenarios() const;
    
	// Track player votes to prevent double voting
	TMap<APlayerState*, FPrimaryAssetId> PlayerVotes;
};
