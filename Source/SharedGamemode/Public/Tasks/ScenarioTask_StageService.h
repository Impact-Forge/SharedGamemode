// Impact Forge LLC 2024

#pragma once

#include "CoreMinimal.h"
#include "ScenarioTask.h"
#include "ScenarioTypes.h"
#include "ScenarioTask_StageService.generated.h"

class UScenarioStage;
/**
 * 
 */
UCLASS(Blueprintable, Abstract)
class SHAREDGAMEMODE_API UScenarioTask_StageService : public UScenarioTask
{
	GENERATED_BODY()
public:
	UScenarioTask_StageService(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Stage transition handling
	UFUNCTION(BlueprintNativeEvent, Category = "Scenario")
	void StageBegun(EScenarioResult PreviousResult, UScenarioStage* PreviousStage);
	virtual void StageBegun_Implementation(EScenarioResult PreviousResult, UScenarioStage* PreviousStage) { }

	UFUNCTION(BlueprintNativeEvent, Category = "Scenario")
	void StageEnded(EScenarioResult StageResult);
	virtual void StageEnded_Implementation(EScenarioResult StageResult) { }

	// Access to current stage
	UFUNCTION(BlueprintPure, Category = "Scenario")
	UScenarioStage* GetStage() const { return OwningStage; }

protected:
	UPROPERTY()
	TObjectPtr<UScenarioStage> OwningStage;
};