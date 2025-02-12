// Impact Forge LLC 2024

#pragma once

#include "CoreMinimal.h"
#include "GameplayScenario.h"
#include "GameplayTagAssetInterface.h"
#include "TagStackContainer.h"
#include "Tasks/ScenarioStage.h"
#include "UObject/Object.h"
#include "ScenarioInstance.generated.h"

// Forward declarations
class UGameplayScenario;
class UScenarioStage;
class UScenarioTask_StageService;
class UScenarioTask_ObjectiveTracker;

// Delegate for scenario completion notification
DECLARE_MULTICAST_DELEGATE_TwoParams(FScenarioEndedDelegate, UScenarioInstance*, bool /*bWasCancelled*/);

/**
 * UScenarioInstance represents a running instance of a scenario.
 * It manages the active state, tasks, and progression through stages.
 */
UCLASS(Blueprintable)
class SHAREDGAMEMODE_API UScenarioInstance : public UObject, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
    UScenarioInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    //~ Begin UObject Interface
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool IsSupportedForNetworking() const override { return true; }
    virtual UWorld* GetWorld() const override;
    //~ End UObject Interface

    //~ Begin IGameplayTagAssetInterface
    virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
    //~ End IGameplayTagAssetInterface

    /** Initialize this instance with a scenario template */
    bool InitScenario(UGameplayScenario* Scenario, const FGameplayTagContainer& InitTags);
    
    /** End the scenario, cleaning up all tasks */
    void EndScenario(bool bCancelled = false);

    /** Check if the scenario is still running */
    UFUNCTION(BlueprintPure, Category = "Scenario")
    bool IsActive() const { return CurrentStage != nullptr; }

    /** Get current scenario state */
    UFUNCTION(BlueprintPure, Category = "Scenario")
    EScenarioState GetState() const { return ScenarioState; }
    
    /** Get the scenario asset template */
    UFUNCTION(BlueprintPure, Category = "Scenario")
    UGameplayScenario* GetScenarioAsset() const { return ScenarioAsset; }

    /** Try to progress to the next stage if objectives are complete */
    bool TryProgressStage();

    /** Get all active objective trackers */
    TArray<UScenarioTask_ObjectiveTracker*> GetCurrentObjectiveTrackers();

    /** Execute function on all stage services */
    void ForEachStageService(TFunctionRef<void(UScenarioTask_StageService*)> Func);
    EScenarioResult EvaluateObjectives();

    //~ Begin Tag Stack System
    /** Add stacks to a tag */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Scenario")
    void AddTagStack(FGameplayTag Tag, int32 StackCount);

    /** Remove stacks from a tag */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Scenario")
    void RemoveTagStack(FGameplayTag Tag, int32 StackCount);

    /** Get current stack count for a tag */
    UFUNCTION(BlueprintCallable, Category = "Scenario")
    int32 GetTagStackCount(FGameplayTag Tag) const;
    //~ End Tag Stack System

    /** 
     * Checks if this instance has authority to make gameplay decisions.
     * Only the server has authority in networked games.
     */
    bool HasAuthority() const;
    
    void OnRep_CurrentStage();

protected:
    /** The scenario template this instance was created from */
    UPROPERTY(Replicated)
    TObjectPtr<UGameplayScenario> ScenarioAsset;

    /** Current state of the scenario */
    UPROPERTY(Replicated)
    EScenarioState ScenarioState;

    /** Currently active stage */
    UPROPERTY(Replicated=OnRep_CurrentStage)
    TObjectPtr<UScenarioStage> CurrentStage;
    
    /** Result of the previous stage */
    UPROPERTY(Replicated)
    EScenarioResult PreviousStageResult;

    /** Tag-based data storage */
    UPROPERTY(Replicated)
    FTagStackContainer TagStacks;

    /** Runtime tags for this instance */
    UPROPERTY(Replicated)
    FGameplayTagContainer RuntimeTags;

    /** Services that run throughout the scenario */
    UPROPERTY()
    TArray<UScenarioTask_StageService*> GlobalServices;

    /** Services specific to current stage */
    UPROPERTY()
    TArray<UScenarioTask_StageService*> StageServices;

    /** Active objective trackers */
    UPROPERTY()
    TArray<UScenarioTask_ObjectiveTracker*> ObjectiveTrackers;

    /** Called when a tag stack count changes */
    void OnTagStackChanged(FGameplayTag Tag, int32 NewCount, int32 OldCount);

private:
    /** Handle stage transitions */
    void EnterStage(UScenarioStage* Stage);
    void ExitStage(UScenarioStage* Stage);
    void ProgressStage_Internal(EScenarioResult Transition);
    float GetStageTransitionDelay() const;

    /** Handle task updates */
    void NotifyTaskUpdate(UScenarioTask_ObjectiveTracker* Task);

    /** Timer for delayed stage transitions */
    FTimerHandle StageProgressionTimer;

    /** Delegate fired when scenario ends */
    FScenarioEndedDelegate OnScenarioEnded;

    /** Allow task system to notify updates */
    friend class UScenarioTask;
    /** Allow subsystem to handle lifecycle */
    friend class UScenarioInstanceSubsystem;
};
