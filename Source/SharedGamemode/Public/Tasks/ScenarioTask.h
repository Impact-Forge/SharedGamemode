// Impact Forge LLC 2024

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ScenarioInstance.h"
#include "UObject/Object.h"
#include "ScenarioTask.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, Abstract)
class SHAREDGAMEMODE_API UScenarioTask : public UObject
{
	GENERATED_BODY()
public:
	UScenarioTask(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Core lifecycle functions that all tasks need
	UFUNCTION(BlueprintNativeEvent, Category = "Scenario")
	void BeginPlay();
	virtual void BeginPlay_Implementation() { }

	UFUNCTION(BlueprintNativeEvent, Category = "Scenario")
	void EndPlay(bool bCancelled);
	virtual void EndPlay_Implementation(bool bCancelled) { }

	// Network support
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual bool IsNameStableForNetworking() const override { return false; }
    
	// World access
	virtual UWorld* GetWorld() const override;

	// Access to owning scenario
	UFUNCTION(BlueprintPure, Category = "Scenario")
	UScenarioInstance* GetScenarioInstance() const;

	// Tag-based data sharing helpers
	template<typename T>
	void ShareData(FGameplayTag Tag, T Value)
	{
		if (UScenarioInstance* Instance = GetScenarioInstance())
		{
			//Instance->SetTagStack(Tag, Value);
		}
	}

	template<typename T>
	T GetSharedData(FGameplayTag Tag, T DefaultValue = T()) const
	{
		if (UScenarioInstance* Instance = GetScenarioInstance())
		{
			return static_cast<T>(Instance->GetTagStackCount(Tag));
		}
		return DefaultValue;
	}

protected:
	// Task result handling
	void SetTaskResult(EScenarioResult NewResult);
    
	UPROPERTY()
	EScenarioResult CurrentResult;

	// Allow instance access to protected members
	friend class UScenarioInstance;
};
