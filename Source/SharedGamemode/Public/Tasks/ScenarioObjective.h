// Impact Forge LLC 2024

#pragma once

#include "CoreMinimal.h"
#include "ScenarioTypes.h"
#include "UObject/Object.h"
#include "ScenarioObjective.generated.h"

class UScenarioTask_ObjectiveTracker;
/**
 * 
 */
UCLASS(Blueprintable)
class SHAREDGAMEMODE_API UScenarioObjective : public UObject
{
	GENERATED_BODY()
public:
	// Objective display info
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	FText ObjectiveName;

	// How this objective is completed
	UPROPERTY(EditAnywhere, Category = "Setup")
	EScenarioCompletionMode CompletionMode;

	// Tasks that track this objective's completion
	UPROPERTY()
	TArray<UScenarioTask_ObjectiveTracker*> ObjectiveTrackers;
};
