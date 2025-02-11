// Impact Forge LLC 2024


#include "Components/ScenarioTransitionComponent.h"

#include "GameplayScenario.h"
#include "ScenarioInstanceSubsystem.h"
#include "Engine/AssetManager.h"
#include "Net/UnrealNetwork.h"


void FScenarioVoteEntry::PreReplicatedRemove(const struct FScenarioVotingState& InArraySerializer)
{
}

void FScenarioVoteEntry::PostReplicatedAdd(const struct FScenarioVotingState& InArraySerializer)
{
	// Notify the voting state has changed
	if (UScenarioTransitionComponent* OwningComponent = Cast<UScenarioTransitionComponent>(InArraySerializer.ScenarioComp))
	{
		OwningComponent->OnRep_VotingState();
	}
}

void FScenarioVoteEntry::PostReplicatedChange(const struct FScenarioVotingState& InArraySerializer)
{
	if (UScenarioTransitionComponent* OwningComponent = Cast<UScenarioTransitionComponent>(InArraySerializer.ScenarioComp))
	{
		OwningComponent->OnRep_VotingState();
	}
}

// Sets default values for this component's properties
UScenarioTransitionComponent::UScenarioTransitionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	VotingDuration = 30.0f;
	NumScenarioOptions = 3;
    
	// Initialize the component reference
	VotingState.ScenarioComp = this;
}

void UScenarioTransitionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UScenarioTransitionComponent, VotingState);
}

void UScenarioTransitionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UScenarioTransitionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (GetOwnerRole() == ROLE_Authority && VotingState.bVotingActive)
	{
		VotingState.VoteTimeRemaining -= DeltaTime;
        
		if (VotingState.VoteTimeRemaining <= 0.0f)
		{
			ProcessVotingResults();
		}
	}
}

void UScenarioTransitionComponent::StartVoting()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	// Reset voting state
	VotingState.bVotingActive = true;
	VotingState.VoteTimeRemaining = VotingDuration;
	PlayerVotes.Empty();

	// Generate new voting options
	GenerateVotingOptions();
}

void UScenarioTransitionComponent::ServerCastVote_Implementation(const FPrimaryAssetId& ScenarioId)
{
	if (!VotingState.bVotingActive)
	{
		return;
	}

	// Get the player's state
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->PlayerState)
	{
		return;
	}

	// Check if player has already voted
	if (FPrimaryAssetId* ExistingVote = PlayerVotes.Find(PC->PlayerState))
	{
		// Remove old vote
		for (FScenarioVoteEntry& Entry : VotingState.VoteOptions)
		{
			if (Entry.ScenarioId == *ExistingVote)
			{
				Entry.VoteCount--;
				break;
			}
		}
	}

	// Record new vote
	PlayerVotes.Add(PC->PlayerState, ScenarioId);

	// Increment vote count
	for (FScenarioVoteEntry& Entry : VotingState.VoteOptions)
	{
		if (Entry.ScenarioId == ScenarioId)
		{
			Entry.VoteCount++;
			break;
		}
	}
}

void UScenarioTransitionComponent::OnRep_VotingState()
{
	OnVotingStateChanged.Broadcast(VotingState);
}

void UScenarioTransitionComponent::GenerateVotingOptions()
{
	VotingState.VoteOptions.Empty();

	// Get all valid scenarios
	TArray<FPrimaryAssetId> ValidScenarios = GetValidScenarios();
    
	// Randomly select options
	while (VotingState.VoteOptions.Num() < FMath::Min(NumScenarioOptions, ValidScenarios.Num()))
	{
		int32 RandomIndex = FMath::RandRange(0, ValidScenarios.Num() - 1);
        
		FScenarioVoteEntry NewEntry;
		NewEntry.ScenarioId = ValidScenarios[RandomIndex];
		NewEntry.VoteCount = 0;
        
		VotingState.VoteOptions.Add(NewEntry);
		ValidScenarios.RemoveAt(RandomIndex);
	}
}

void UScenarioTransitionComponent::ProcessVotingResults()
{
	if (!VotingState.bVotingActive)
	{
		return;
	}

	// Find winning scenario
	FPrimaryAssetId WinningScenario;
	int32 HighestVotes = -1;

	for (const FScenarioVoteEntry& Entry : VotingState.VoteOptions)
	{
		if (Entry.VoteCount > HighestVotes)
		{
			HighestVotes = Entry.VoteCount;
			WinningScenario = Entry.ScenarioId;
		}
	}

	// If no votes, pick random
	if (HighestVotes == 0 && VotingState.VoteOptions.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, VotingState.VoteOptions.Num() - 1);
		WinningScenario = VotingState.VoteOptions[RandomIndex].ScenarioId;
	}

	// End voting
	VotingState.bVotingActive = false;
    
	// Broadcast result
	OnNextScenarioSelected.Broadcast(WinningScenario);

	// Transition to new scenario
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (UScenarioInstanceSubsystem* ScenarioSystem = GameInstance->GetSubsystem<UScenarioInstanceSubsystem>())
		{
			UGameplayScenario* NextScenario = Cast<UGameplayScenario>(UAssetManager::Get().GetPrimaryAssetObject(WinningScenario));
			if (NextScenario)
			{
				ScenarioSystem->SetPendingScenario(NextScenario);
				ScenarioSystem->TransitionToPendingScenario(true);
			}
		}
	}
}

TArray<FPrimaryAssetId> UScenarioTransitionComponent::GetValidScenarios() const
{
	TArray<FPrimaryAssetId> ValidScenarios;
    
	// Get all scenario assets
	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FPrimaryAssetId> AllScenarios;
    
	AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType("GameplayScenario"), AllScenarios);

	// Filter based on tag query if set
	for (const FPrimaryAssetId& ScenarioId : AllScenarios)
	{
		if (UGameplayScenario* Scenario = Cast<UGameplayScenario>(AssetManager.GetPrimaryAssetObject(ScenarioId)))
		{
			if (!ScenarioFilter.IsEmpty())
			{
				FGameplayTagContainer Tags;
				Scenario->GetOwnedGameplayTags(Tags);
                
				if (!ScenarioFilter.Matches(Tags))
				{
					continue;
				}
			}
            
			ValidScenarios.Add(ScenarioId);
		}
	}

	return ValidScenarios;
}