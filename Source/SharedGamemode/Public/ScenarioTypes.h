// Impact Forge LLC 2024

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EScenarioState : uint8
{
	None,       // Initial state/inactive
	Active,     // Scenario is running
	Success,    // Completed successfully  
	Failure,    // Failed to complete
	Cancelled   // Terminated early
};

UENUM(BlueprintType)
enum class EScenarioResult : uint8
{
	InProgress,     // Task/Stage still running
	Success,        // Completed successfully
	Failure,        // Failed to complete
	None           // Not started/invalid
};

UENUM(BlueprintType)
enum class EScenarioCompletionMode : uint8
{
	// Succeed if ANY tracker succeeds
	AnySuccess,
    
	// Succeed only if ALL trackers succeed
	AllSuccess
};