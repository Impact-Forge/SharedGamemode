// Impact Forge LLC 2024

#pragma once

#include "CoreMinimal.h"
#include "ScenarioTypes.h"
#include "UObject/Object.h"
#include "ScenarioStage.generated.h"

class UScenarioUIObject;
class UScenarioTask_StageService;
class UScenarioObjective;
/**
 * 
 */
UCLASS(Blueprintable)
class SHAREDGAMEMODE_API UScenarioStage : public UObject
{
	GENERATED_BODY()

public:
	// Stage identification
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
	FText StageName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
	FText StageDescription;

	// Stage completion settings
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
	EScenarioCompletionMode CompletionMode;

	// How long to wait after completing objectives before transitioning
	UPROPERTY(EditAnywhere, Category = "Stage")
	float StageCompletionDelay;

	// Next stage branching based on success/failure
	UPROPERTY(VisibleAnywhere, Category = "Stage")
	UScenarioStage* NextStage_Success;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	UScenarioStage* NextStage_Failure;

	// Stage components
	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TArray<UScenarioObjective*> Objectives;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TArray<UScenarioTask_StageService*> StageServices;

	// Optional UI data for this stage
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI", Instanced)
	TObjectPtr<UScenarioUIObject> UIData;
};
