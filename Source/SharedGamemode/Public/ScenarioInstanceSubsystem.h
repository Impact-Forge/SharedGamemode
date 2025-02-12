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
#include "ScenarioInstance.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ScenarioInstanceSubsystem.generated.h"

class UGameplayScenario;
class ULevelStreamingDynamic;
class UGameplaySA_ChangeMap;
class AScenarioReplicationProxy;

// Struct to represent scenario state change
USTRUCT()
struct FScenarioStateChanged
{
	GENERATED_BODY()

public:
	UScenarioInstance* Instance;
	EScenarioState NewState;
	EScenarioState OldState;

	FScenarioStateChanged(): Instance(nullptr), NewState(), OldState()
	{
	}

	FScenarioStateChanged(UScenarioInstance* InInstance, EScenarioState InNewState, EScenarioState InOldState)
		: Instance(InInstance)
		, NewState(InNewState)
		, OldState(InOldState)
	{}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FScenarioDelegate, UGameplayScenario*, Scenario);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnScenarioStateChanged, const FScenarioStateChanged&);


/**
 * 
 */
UCLASS()
class SHAREDGAMEMODE_API UScenarioInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UScenarioInstanceSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Scenario")
	UScenarioInstance* StartScenario(UGameplayScenario* ScenarioAsset, const FGameplayTagContainer& Tags);

	UFUNCTION(BlueprintCallable, Category = "Scenario")
	void CancelScenario(UScenarioInstance* Instance);
	
	UFUNCTION(BlueprintCallable, Category="Scenario")
	virtual void SetPendingScenario(UGameplayScenario* Scenairo);
	UFUNCTION(BlueprintCallable, Category = "Scenario")
	virtual void TransitionToPendingScenario(bool bForce = false);

	// Instance iteration
	void ForEachScenario(TFunctionRef<void(const UScenarioInstance*)> Pred) const;
	void ForEachScenario_Mutable(TFunctionRef<void(UScenarioInstance*)> Pred);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scenario")
	TArray<UGameplayScenario*> ActiveScenarios;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scenario")
	UGameplayScenario* PendingScenario;

	UPROPERTY()
	UGameplayScenario* MapTransitionScenario;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Scenario")
	bool bBecomeListenServerFromStandalone;

	UPROPERTY(BlueprintAssignable)
	FScenarioDelegate OnScenarioActivated;
	
	UPROPERTY(BlueprintAssignable)
	FScenarioDelegate OnScenarioDeactivated;

	// Add this to your existing subsystem class
	FOnScenarioStateChanged OnScenarioStateChanged;

	// Replace BroadcastMessage with a method that uses the delegate
	void NotifyScenarioStateChanged(const FScenarioStateChanged& StateChange)
	{
		OnScenarioStateChanged.Broadcast(StateChange);
	}

	// Method can use forward-declared type
	void SetReplicationProxy(AScenarioReplicationProxy* Proxy);

	
	friend class UGameplaySA_ActivateScenario;
	friend class UGameplaySA_DeactivateScenario;
	friend class UGamestateScenarioComponent;
protected:
	virtual void PreActivateScenario(FPrimaryAssetId ScenarioAsset, bool bForce);
	virtual void PreActivateScenario(UGameplayScenario* Scenario, bool bForce);

	virtual void ActivateScenario(FPrimaryAssetId ScenarioAsset, bool bForce);
	virtual void ActivateScenario(UGameplayScenario* Scenario, bool bForce);

	virtual void DeactivateScenario(UGameplayScenario* Scenario);
	virtual void DeactivateScenario(FPrimaryAssetId ScenarioAsset);

	void TearDownActiveScenarios();

	virtual bool IsScenarioActive(UGameplayScenario* Scenario) const;

	virtual void OnPostLoadMap(UWorld* World);
	virtual void OnPreLoadMap(const FString& MapName);

	void StartActivatingScenario(UGameplayScenario* Scenario, bool bForce);
	void FinishActivatingScenario(UGameplayScenario* Scenario, bool bForce);

	void TransitionToWorld(FPrimaryAssetId World);

	// Active scenario tracking
	UPROPERTY()
	TArray<UScenarioInstance*> ScenarioInstances;

	// Replication support
	UPROPERTY()
	AScenarioReplicationProxy* ReplicationProxy;

private:
	void OnScenarioEnded(UScenarioInstance* Instance, bool bWasCancelled);
	void NotifyAddedScenarioFromReplication(UScenarioInstance* Instance);
	void NotifyRemovedScenarioFromReplication(UScenarioInstance* Instance);
};
