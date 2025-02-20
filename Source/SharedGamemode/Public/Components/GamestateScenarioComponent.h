/*
Copyright 2021 Empires Team

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "Engine/NetSerialization.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "GamestateScenarioComponent.generated.h"

class UGameplayScenario;

USTRUCT()
struct FGameplayScenarioNetworkArrayItem : public FFastArraySerializerItem
{
	GENERATED_BODY()
public:
	FGameplayScenarioNetworkArrayItem()
	{
		Scenario = nullptr;
		PrevScenario = nullptr;
		bPendingRemoval = false;
	}

	UPROPERTY()
	UGameplayScenario* Scenario;

	TWeakObjectPtr<UGameplayScenario> PrevScenario;

	bool bPendingRemoval;
	
	void PreReplicatedRemove(const struct FGameplayScenarioNetworkArray& InArraySerializer);
	void PostReplicatedAdd(const struct FGameplayScenarioNetworkArray& InArraySerializer);
	void PostReplicatedChange(const struct FGameplayScenarioNetworkArray& InArraySerializer);
};


USTRUCT()
struct FGameplayScenarioNetworkArray : public FFastArraySerializer 
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<FGameplayScenarioNetworkArrayItem> Items;

	UPROPERTY()
	UGamestateScenarioComponent* ScenarioComp;
	

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FGameplayScenarioNetworkArrayItem, FGameplayScenarioNetworkArray>(Items, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits< FGameplayScenarioNetworkArray > : public TStructOpsTypeTraitsBase2< FGameplayScenarioNetworkArray >
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

/**
 * 
 */
UCLASS()
class SHAREDGAMEMODE_API UGamestateScenarioComponent : public UGameStateComponent
{
	GENERATED_BODY()
public:
	UGamestateScenarioComponent(const FObjectInitializer& ObjectInitializer);
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	
	// Server-side functions
	UFUNCTION(Server, Reliable)
	void ServerActivateScenario(UGameplayScenario* Scenario);

	UFUNCTION(Server, Reliable)
	void ServerDeactivateScenario(UGameplayScenario* Scenario);
	
	UFUNCTION()
	virtual void OnScenarioActivated(UGameplayScenario* Scenario);
	UFUNCTION()
	virtual void OnScenarioDeactivated(UGameplayScenario* Scenario);

	UFUNCTION(BlueprintPure, Category = "Scenarios")
	bool IsScenarioActive(UGameplayScenario* Scenario) const;

	UPROPERTY(Replicated)
	FGameplayScenarioNetworkArray Scenarios;

	friend struct FGameplayScenarioNetworkArrayItem;
	
protected:
	void ActivateScenarioLocally(UGameplayScenario* Scenario);
	void DeactivateScenarioLocally(UGameplayScenario* Scenario);

	// Cleanup pending scenarios
	void CleanupPendingScenarios();

	// Track authority
	bool bHasAuthority;

private:
	// Helper to find scenario in network array
	int32 FindScenarioIndex(UGameplayScenario* Scenario) const;

	// Timer handle for cleanup
	FTimerHandle CleanupTimerHandle;
};
