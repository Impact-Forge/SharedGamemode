// Impact Forge LLC 2024

#pragma once

#include "CoreMinimal.h"
#include "ScenarioTask.h"
#include "ScenarioTypes.h"
#include "ScenarioTask_ObjectiveTracker.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, Abstract)
class SHAREDGAMEMODE_API UScenarioTask_ObjectiveTracker : public UScenarioTask
{
	GENERATED_BODY()
public:
	UScenarioTask_ObjectiveTracker(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Objective state management
	UFUNCTION(BlueprintCallable, Category = "Scenario")
	void MarkSuccess() { SetTaskResult(EScenarioResult::Success); }

	UFUNCTION(BlueprintCallable, Category = "Scenario")
	void MarkFailure() { SetTaskResult(EScenarioResult::Failure); }

	// Access to owning objective
	UFUNCTION(BlueprintPure, Category = "Scenario")
	UScenarioObjective* GetObjective() const { return Objective; }

	UFUNCTION(BlueprintPure, Category = "Scenario")
	EScenarioResult GetTrackerState() const { return CurrentResult; }

protected:
	UPROPERTY()
	TObjectPtr<UScenarioObjective> Objective;

	// Change notification
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTrackerUpdated, UScenarioTask_ObjectiveTracker*);
	FOnTrackerUpdated OnTrackerStateUpdated;
};
