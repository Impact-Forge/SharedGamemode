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


#include "GameplaySA_ApplyGASPrimitives.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "EngineUtils.h"

void UGameplaySA_ApplyGASPrimitives::OnScenarioActivated(UScenarioInstanceSubsystem* ScenarioSubsystem)
{
	// Process each target configuration
	for (const FGASPrimitivesTarget& Target : Targets)
	{
		// Find matching actors
		TArray<AActor*> MatchingActors;
		GetMatchingActors(Target.ActorQuery, MatchingActors);

		// Apply GAS primitives to each matching actor
		for (AActor* Actor : MatchingActors)
		{
			if (!Actor)
			{
				continue;
			}

			// Get the actor's ability system component
			UAbilitySystemComponent* ASC = Actor->FindComponentByClass<UAbilitySystemComponent>();
			if (!ASC)
			{
				if (IAbilitySystemInterface* ASInterface = Cast<IAbilitySystemInterface>(Actor))
				{
					ASC = ASInterface->GetAbilitySystemComponent();
				}
			}

			if (!ASC)
			{
				continue;
			}

			// Grant each ability set
			TArray<FGSCAbilitySetHandle>& ActorHandles = GrantedAbilitySetHandles.FindOrAdd(Actor);
            
			for (const TSoftObjectPtr<UGSCAbilitySet>& AbilitySetPtr : Target.AbilitySets)
			{
				const UGSCAbilitySet* AbilitySet = AbilitySetPtr.Get();
				if (!AbilitySet)
				{
					continue;
				}

				FGSCAbilitySetHandle Handle;
				if (AbilitySet->GrantToAbilitySystem(ASC, Handle))
				{
					ActorHandles.Add(Handle);
				}
			}
		}
	}
}

void UGameplaySA_ApplyGASPrimitives::OnScenarioDeactivated(UScenarioInstanceSubsystem* ScenarioSubsystem,
	bool bTearDown)
{
	// Remove all granted ability sets
	for (auto& KVP : GrantedAbilitySetHandles)
	{
		if (AActor* Actor = KVP.Key.Get())
		{
			UAbilitySystemComponent* ASC = Actor->FindComponentByClass<UAbilitySystemComponent>();
			if (!ASC)
			{
				if (IAbilitySystemInterface* ASInterface = Cast<IAbilitySystemInterface>(Actor))
				{
					ASC = ASInterface->GetAbilitySystemComponent();
				}
			}

			if (!ASC)
			{
				continue;
			}

			// Remove each granted ability set
			for (FGSCAbilitySetHandle& Handle : KVP.Value)
			{
				UGSCAbilitySet::RemoveFromAbilitySystem(ASC, Handle);
			}
		}
	}

	GrantedAbilitySetHandles.Empty();
}

void UGameplaySA_ApplyGASPrimitives::GetMatchingActors(const FGameplayTagQuery& Query, TArray<AActor*>& OutActors) const
{
	if (!GetWorld())
	{
		return;
	}

	// Iterate all actors in world
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		// Check if actor implements tag interface
		IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Actor);
		if (!TagInterface)
		{
			continue;
		}

		// Get actor's tags and check against query
		FGameplayTagContainer ActorTags;
		TagInterface->GetOwnedGameplayTags(ActorTags);
        
		if (Query.Matches(ActorTags))
		{
			OutActors.Add(Actor);
		}
	}
}
