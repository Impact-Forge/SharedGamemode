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
#include "GameplayScenarioAction.h"
#include "GameplayTagContainer.h"
#include "Abilities/GSCAbilitySet.h"
#include "GameplaySA_ApplyGASPrimitives.generated.h"


/** Defines a target for GAS primitives and what to apply */
USTRUCT(BlueprintType)
struct FGASPrimitivesTarget 
{
	GENERATED_BODY()

	/** Tag query to find actors to apply GAS primitives to */
	UPROPERTY(EditAnywhere, Category = "GAS")
	FGameplayTagQuery ActorQuery;

	/** Ability sets to grant */
	UPROPERTY(EditAnywhere, Category = "GAS")
	TArray<TSoftObjectPtr<UGSCAbilitySet>> AbilitySets;
};

/**
 * Scenario action that applies GAS primitives (Abilities, Attributes, Effects) to specified actors
 */
UCLASS()
class SHAREDGAMEMODE_API UGameplaySA_ApplyGASPrimitives : public UGameplayScenarioAction
{
	GENERATED_BODY()
public:
	/** List of GAS primitive targets and what to apply to them */
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<FGASPrimitivesTarget> Targets;

	/** Map of target actors to their granted ability set handles */
	TMap<TWeakObjectPtr<AActor>, TArray<FGSCAbilitySetHandle>> GrantedAbilitySetHandles;

	//~ Begin UGameplayScenarioAction interface
	virtual void OnScenarioActivated(UScenarioInstanceSubsystem* ScenarioSubsystem) override;
	virtual void OnScenarioDeactivated(UScenarioInstanceSubsystem* ScenarioSubsystem, bool bTearDown = false) override;
	//~ End UGameplayScenarioAction interface

private:
	/** Finds all actors matching the tag query */
	void GetMatchingActors(const FGameplayTagQuery& Query, TArray<AActor*>& OutActors) const;
};
