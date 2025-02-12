// Impact Forge LLC 2024

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ScenarioUIObject.generated.h"

class UScenarioStage;
/**
 * 
 */
UCLASS(EditInlineNew, Abstract)
class SHAREDGAMEMODE_API UScenarioUIObject : public UObject
{
	GENERATED_BODY()
public:
	// Base class doesn't define specific UI data
	// This lets derived classes define their own UI requirements
    
	// Optional method to implement custom UI initialization
	UFUNCTION(BlueprintNativeEvent, Category = "UI")
	void InitializeUI();
	virtual void InitializeUI_Implementation() {}

	// Get the owning stage 
	UFUNCTION(BlueprintPure, Category = "UI")
	UScenarioStage* GetStage() const;
};
