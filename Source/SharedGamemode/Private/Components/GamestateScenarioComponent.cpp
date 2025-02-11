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


#include "Components/GamestateScenarioComponent.h"
#include "ScenarioInstanceSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "GameplayScenario.h"

UGamestateScenarioComponent::UGamestateScenarioComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Scenarios.ScenarioComp = this;
	SetIsReplicatedByDefault(true);
	bHasAuthority = false;
}

void UGamestateScenarioComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGamestateScenarioComponent, Scenarios);
}


void UGamestateScenarioComponent::OnRegister()
{
	Super::OnRegister();

	// Determine authority
	UWorld* World = GetWorld();
	bHasAuthority = World && (World->GetNetMode() == NM_DedicatedServer || World->GetNetMode() == NM_ListenServer);

	if (UGameInstance* GameInstance = GetGameInstance<UGameInstance>())
	{
		if (UScenarioInstanceSubsystem* Subsys = GameInstance->GetSubsystem<UScenarioInstanceSubsystem>())
		{
			Subsys->OnScenarioActivated.AddDynamic(this, &ThisClass::OnScenarioActivated);
			Subsys->OnScenarioDeactivated.AddDynamic(this, &ThisClass::OnScenarioDeactivated);

			// Only initialize active scenarios on authority
			if (bHasAuthority)
			{
				for (UGameplayScenario* Scenario : Subsys->ActiveScenarios)
				{
					OnScenarioActivated(Scenario);
				}
			}
		}
	}
}

void UGamestateScenarioComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Start periodic cleanup if we have authority
	if (bHasAuthority)
	{
		GetWorld()->GetTimerManager().SetTimer(
			CleanupTimerHandle,
			this,
			&UGamestateScenarioComponent::CleanupPendingScenarios,
			1.0f,
			true
		);
	}
}

void UGamestateScenarioComponent::ServerActivateScenario_Implementation(UGameplayScenario* Scenario)
{
	if (!bHasAuthority || !IsValid(Scenario))
	{
		return;
	}

	// Check if scenario is already active
	if (IsScenarioActive(Scenario))
	{
		return;
	}

	FGameplayScenarioNetworkArrayItem Item;
	Item.Scenario = Scenario;
    
	Scenarios.Items.Add(Item);
	Scenarios.MarkArrayDirty();
	Scenarios.MarkItemDirty(Item);
}

void UGamestateScenarioComponent::ServerDeactivateScenario_Implementation(UGameplayScenario* Scenario)
{
	if (!bHasAuthority || !IsValid(Scenario))
	{
		return;
	}

	int32 Index = FindScenarioIndex(Scenario);
	if (Index != INDEX_NONE)
	{
		Scenarios.Items[Index].bPendingRemoval = true;
		Scenarios.MarkArrayDirty();
	}
}

bool UGamestateScenarioComponent::IsScenarioActive(UGameplayScenario* Scenario) const
{
	return FindScenarioIndex(Scenario) != INDEX_NONE;
}

void UGamestateScenarioComponent::CleanupPendingScenarios()
{
	if (!bHasAuthority)
	{
		return;
	}

	bool bNeedsCleanup = false;
	for (int32 i = Scenarios.Items.Num() - 1; i >= 0; --i)
	{
		if (Scenarios.Items[i].bPendingRemoval)
		{
			Scenarios.Items.RemoveAt(i);
			bNeedsCleanup = true;
		}
	}

	if (bNeedsCleanup)
	{
		Scenarios.MarkArrayDirty();
	}
}

int32 UGamestateScenarioComponent::FindScenarioIndex(UGameplayScenario* Scenario) const
{
	return Scenarios.Items.IndexOfByPredicate([Scenario](const FGameplayScenarioNetworkArrayItem& Item)
   {
	   return Item.Scenario == Scenario && !Item.bPendingRemoval;
   });
}

void UGamestateScenarioComponent::OnScenarioActivated(UGameplayScenario* Scenario)
{
	if (bHasAuthority)
	{
		ServerActivateScenario(Scenario);
	}
}

void UGamestateScenarioComponent::OnScenarioDeactivated(UGameplayScenario* Scenario)
{
	if (bHasAuthority)
	{
		ServerDeactivateScenario(Scenario);
	}
}


void UGamestateScenarioComponent::ActivateScenarioLocally(UGameplayScenario* Scenario)
{
	if (!IsValid(Scenario))
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance<UGameInstance>())
	{
		if (UScenarioInstanceSubsystem* Subsys = GameInstance->GetSubsystem<UScenarioInstanceSubsystem>())
		{
			Subsys->ActivateScenario(Scenario, false);
		}
	}
}

void UGamestateScenarioComponent::DeactivateScenarioLocally(UGameplayScenario* Scenario)
{
	if (!IsValid(Scenario))
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance<UGameInstance>())
	{
		if (UScenarioInstanceSubsystem* Subsys = GameInstance->GetSubsystem<UScenarioInstanceSubsystem>())
		{
			Subsys->DeactivateScenario(Scenario);
		}
	}
}

void FGameplayScenarioNetworkArrayItem::PreReplicatedRemove(const struct FGameplayScenarioNetworkArray& InArraySerializer)
{
	if (IsValid(InArraySerializer.ScenarioComp))
	{
		InArraySerializer.ScenarioComp->DeactivateScenarioLocally(Scenario);
		PrevScenario = nullptr;
	}
}

void FGameplayScenarioNetworkArrayItem::PostReplicatedAdd(const struct FGameplayScenarioNetworkArray& InArraySerializer)
{
	if (IsValid(InArraySerializer.ScenarioComp))
	{
		InArraySerializer.ScenarioComp->ActivateScenarioLocally(Scenario);
		PrevScenario = Scenario;
	}
}

void FGameplayScenarioNetworkArrayItem::PostReplicatedChange(const struct FGameplayScenarioNetworkArray& InArraySerializer)
{
	if (IsValid(InArraySerializer.ScenarioComp))
	{
		UGameplayScenario* Prev = PrevScenario.Get();
		PrevScenario = Scenario;

		if (IsValid(Prev))
		{
			InArraySerializer.ScenarioComp->DeactivateScenarioLocally(Prev);
		}
		if (IsValid(Scenario))
		{
			InArraySerializer.ScenarioComp->ActivateScenarioLocally(Scenario);
		}
	}
}
