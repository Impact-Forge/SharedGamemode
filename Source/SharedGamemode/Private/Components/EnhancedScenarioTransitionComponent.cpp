// Impact Forge LLC 2024


#include "Components/EnhancedScenarioTransitionComponent.h"

#include "GameplayScenario.h"
#include "ScenarioInstanceSubsystem.h"
#include "ScenarioPersistenceManager.h"
#include "AbilitySystem/Phases/BSGamePhaseSubsystem.h"
#include "Engine/AssetManager.h"

class UBSGamePhaseSubsystem;

UEnhancedScenarioTransitionComponent::UEnhancedScenarioTransitionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	MinimumVoteWeight = 0.5f;
	MaximumVoteWeight = 2.0f;
	PerformanceWeightMultiplier = 0.1f;
}

void UEnhancedScenarioTransitionComponent::ServerVetoScenario_Implementation(const FPrimaryAssetId& ScenarioId)
{
	if (!VotingState.bVotingActive)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->PlayerState)
	{
		return;
	}

	// Check if player has already vetoed
	FPlayerVoteWeight* PlayerWeight = PlayerWeights.Find(PC->PlayerState);
	if (!PlayerWeight || PlayerWeight->bHasVetoed)
	{
		return;
	}

	// Update veto count
	for (FEnhancedVoteEntry& Entry : static_cast<FEnhancedVotingState*>(&VotingState)->EnhancedVoteOptions)
	{
		if (Entry.ScenarioId == ScenarioId)
		{
			Entry.VetoCount++;
			PlayerWeight->bHasVetoed = true;
			break;
		}
	}
}

void UEnhancedScenarioTransitionComponent::ServerUpdatePlayerPerformance_Implementation(APlayerState* PlayerState,
	float PerformanceScore)
{
	if (!PlayerState)
	{
		return;
	}

	FPlayerVoteWeight& Weight = PlayerWeights.FindOrAdd(PlayerState);
    
	// Calculate new performance multiplier
	float NewMultiplier = FMath::Clamp(PerformanceScore * PerformanceWeightMultiplier, 
		MinimumVoteWeight, MaximumVoteWeight);
    
	Weight.PerformanceMultiplier = NewMultiplier;
    
	// Update vote weights if voting is active
	if (VotingState.bVotingActive)
	{
		UpdateVoteWeights();
	}
}

void UEnhancedScenarioTransitionComponent::GenerateVotingOptions()
{
	// Get persistence manager
	UScenarioPersistenceManager* PersistenceManager = GetGameInstance<UGameInstance>()->GetSubsystem<UScenarioPersistenceManager>();
	if (!PersistenceManager)
	{
		Super::GenerateVotingOptions();
		return;
	}

	// Get weighted options considering rotation and popularity
	TArray<FPrimaryAssetId> WeightedOptions = PersistenceManager->GetWeightedScenarioOptions(NumScenarioOptions);
    
	// Convert to enhanced vote entries
    FEnhancedVotingState* EnhancedState = static_cast<FEnhancedVotingState*>(&VotingState);
	EnhancedState->EnhancedVoteOptions.Empty();

	for (const FPrimaryAssetId& ScenarioId : WeightedOptions)
	{
		FEnhancedVoteEntry NewEntry;
		NewEntry.ScenarioId = ScenarioId;
		NewEntry.VoteCount = 0;
		NewEntry.WeightedVotes = 0.0f;
		NewEntry.VetoCount = 0;
        
		EnhancedState->EnhancedVoteOptions.Add(NewEntry);
	}

	// Apply rotation rules
	ApplyRotationRules();
}

void UEnhancedScenarioTransitionComponent::ProcessVotingResults()
{
	   if (!VotingState.bVotingActive)
    {
        return;
    }

	FEnhancedVotingState* EnhancedState = static_cast<FEnhancedVotingState*>(&VotingState);
    
    // Find winning scenario considering vetos and weighted votes
    FPrimaryAssetId WinningScenario;
    float HighestWeightedVotes = -1.0f;

    for (const FEnhancedVoteEntry& Entry : EnhancedState->EnhancedVoteOptions)
    {
        // Skip vetoed scenarios
        if (Entry.VetoCount >= EnhancedState->VetoThreshold)
        {
            continue;
        }

        if (Entry.WeightedVotes > HighestWeightedVotes)
        {
            HighestWeightedVotes = Entry.WeightedVotes;
            WinningScenario = Entry.ScenarioId;
        }
    }

    // If all scenarios were vetoed or no votes, pick random non-vetoed
    if (!WinningScenario.IsValid())
    {
        TArray<FEnhancedVoteEntry> ValidOptions;
        for (const FEnhancedVoteEntry& Entry : EnhancedState->EnhancedVoteOptions)
        {
            if (Entry.VetoCount < EnhancedState->VetoThreshold)
            {
                ValidOptions.Add(Entry);
            }
        }

        if (ValidOptions.Num() > 0)
        {
            int32 RandomIndex = FMath::RandRange(0, ValidOptions.Num() - 1);
            WinningScenario = ValidOptions[RandomIndex].ScenarioId;
        }
    }

    // Update persistence
    if (UScenarioPersistenceManager* PersistenceManager = GetGameInstance<UGameInstance>()->GetSubsystem<UScenarioPersistenceManager>())
    {
        PersistenceManager->UpdatePlayCount(WinningScenario);
    }

    // End voting and transition
    VotingState.bVotingActive = false;
    OnNextScenarioSelected.Broadcast(WinningScenario);

    // Transition to new scenario (same as base class)
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

float UEnhancedScenarioTransitionComponent::CalculatePlayerVoteWeight(APlayerState* PlayerState) const
{
	if (const FPlayerVoteWeight* Weight = PlayerWeights.Find(PlayerState))
	{
		return Weight->BaseWeight * Weight->PerformanceMultiplier;
	}
	return 1.0f;
}

void UEnhancedScenarioTransitionComponent::UpdateVoteWeights()
{
	FEnhancedVotingState* EnhancedState = static_cast<FEnhancedVotingState*>(&VotingState);
    
	// Reset all weighted votes before recalculating
	for (FEnhancedVoteEntry& Entry : EnhancedState->EnhancedVoteOptions)
	{
		Entry.WeightedVotes = 0.0f;
	}

	// Calculate weighted votes based on player performance
	for (const auto& PlayerVotePair : PlayerVotes)
	{
		APlayerState* PlayerState = PlayerVotePair.Key;
		FPrimaryAssetId VotedScenario = PlayerVotePair.Value;
        
		// Calculate this player's vote weight
		float Weight = CalculatePlayerVoteWeight(PlayerState);
        
		// Apply weighted vote to the appropriate scenario
		for (FEnhancedVoteEntry& Entry : EnhancedState->EnhancedVoteOptions)
		{
			if (Entry.ScenarioId == VotedScenario)
			{
				Entry.WeightedVotes += Weight;
				break;
			}
		}
	}
}

void UEnhancedScenarioTransitionComponent::InitializePlayerWeight(APlayerState* PlayerState)
{
	// Only initialize if not already present
	if (!PlayerWeights.Contains(PlayerState))
	{
		FPlayerVoteWeight NewWeight;
		NewWeight.BaseWeight = 1.0f;
		NewWeight.PerformanceMultiplier = 1.0f;
		NewWeight.bHasVetoed = false;
        
		PlayerWeights.Add(PlayerState, NewWeight);
	}
}

bool UEnhancedScenarioTransitionComponent::IsScenarioVetoed(const FPrimaryAssetId& ScenarioId) const
{
	const FEnhancedVotingState* EnhancedState = static_cast<const FEnhancedVotingState*>(&VotingState);
    
	// Check if scenario has reached veto threshold
	for (const FEnhancedVoteEntry& Entry : EnhancedState->EnhancedVoteOptions)
	{
		if (Entry.ScenarioId == ScenarioId)
		{
			return Entry.VetoCount >= EnhancedState->VetoThreshold;
		}
	}
    
	return false;
}

void UEnhancedScenarioTransitionComponent::ApplyRotationRules()
{
	UScenarioPersistenceManager* PersistenceManager = GetGameInstance<UGameInstance>()->GetSubsystem<UScenarioPersistenceManager>();
	if (!PersistenceManager)
	{
		return;
	}

	FEnhancedVotingState* EnhancedState = static_cast<FEnhancedVotingState*>(&VotingState);
	TArray<FEnhancedVoteEntry> FilteredOptions;

	// Filter out scenarios that don't meet rotation rules
	for (const FEnhancedVoteEntry& Entry : EnhancedState->EnhancedVoteOptions)
	{
		if (PersistenceManager->IsScenarioAllowedInRotation(Entry.ScenarioId))
		{
			FilteredOptions.Add(Entry);
		}
	}

	// If we filtered out too many options, add some back from the rotation pool
	if (FilteredOptions.Num() < NumScenarioOptions / 2)
	{
		TArray<FPrimaryAssetId> RotationOptions = PersistenceManager->GetNextRotationOptions();
        
		for (const FPrimaryAssetId& ScenarioId : RotationOptions)
		{
			if (FilteredOptions.Num() >= NumScenarioOptions)
			{
				break;
			}

			// Only add if not already in filtered options
			bool bAlreadyExists = false;
			for (const FEnhancedVoteEntry& Entry : FilteredOptions)
			{
				if (Entry.ScenarioId == ScenarioId)
				{
					bAlreadyExists = true;
					break;
				}
			}

			if (!bAlreadyExists)
			{
				FEnhancedVoteEntry NewEntry;
				NewEntry.ScenarioId = ScenarioId;
				FilteredOptions.Add(NewEntry);
			}
		}
	}

	// Update voting options with filtered list
	EnhancedState->EnhancedVoteOptions = FilteredOptions;
}

UScenarioGameModeComponent::UScenarioGameModeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = false;
    bWantsInitializeComponent = true;

    // Set default end game phase tag - can be overridden in BP
    EndGamePhaseTag = FGameplayTag::RequestGameplayTag(FName("Game.EndGame"));
}

void UScenarioGameModeComponent::OnRegister()
{
    Super::OnRegister();

    // Store reference to owning GameMode
    OwningGameMode = Cast<AGameModeBase>(GetOwner());
}

void UScenarioGameModeComponent::BeginPlay()
{
    Super::BeginPlay();

    // Get phase subsystem
    if (UWorld* World = GetWorld())
    {
        PhaseSubsystem = UWorld::GetSubsystem<UBSGamePhaseSubsystem>(World);
        if (PhaseSubsystem)
        {
            // Register for end game phase
            PhaseSubsystem->WhenPhaseStartsOrIsActive(
                EndGamePhaseTag,
                EPhaseTagMatchType::ExactMatch,
                FBSGamePhaseTagDelegate::CreateUObject(this, &ThisClass::HandlePhaseChange)
            );
        }
    }
}

void UScenarioGameModeComponent::OnUnregister()
{
    PhaseSubsystem = nullptr;
    Super::OnUnregister();
}

void UScenarioGameModeComponent::UpdatePlayerPerformance(APlayerState* PlayerState, float Score)
{
    if (!OwningGameMode.IsValid() || !PlayerState)
    {
        return;
    }

    if (AGameStateBase* GameState = OwningGameMode->GetGameState<AGameStateBase>())
    {
        if (UEnhancedScenarioTransitionComponent* TransitionComp = GameState->FindComponentByClass<UEnhancedScenarioTransitionComponent>())
        {
            TransitionComp->ServerUpdatePlayerPerformance(PlayerState, Score);
        }
    }
}

void UScenarioGameModeComponent::StartScenarioVoting()
{
    if (!OwningGameMode.IsValid())
    {
        return;
    }

    if (AGameStateBase* GameState = OwningGameMode->GetGameState<AGameStateBase>())
    {
        if (UEnhancedScenarioTransitionComponent* TransitionComp = GameState->FindComponentByClass<UEnhancedScenarioTransitionComponent>())
        {
            TransitionComp->StartVoting();
        }
    }
}

void UScenarioGameModeComponent::HandlePhaseChange(const FGameplayTag& PhaseTag)
{
    // Start voting when we enter the end game phase
    if (PhaseTag == EndGamePhaseTag)
    {
        StartScenarioVoting();
    }
}