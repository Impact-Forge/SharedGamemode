// Impact Forge LLC 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScenarioReplicationProxy.generated.h"

class UScenarioInstance;
class UScenarioInstanceSubsystem;

UCLASS(NotBlueprintable, NotPlaceable)
class SHAREDGAMEMODE_API AScenarioReplicationProxy : public AActor
{
	GENERATED_BODY()
public:
	AScenarioReplicationProxy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	// Add a replicated scenario instance
	void AddReplicatedInstance(UScenarioInstance* Instance);

	// Remove a replicated scenario instance
	void RemoveReplicatedInstance(UScenarioInstance* Instance);

	// Get the owning subsystem
	UFUNCTION(BlueprintPure, Category = "Scenario")
	UScenarioInstanceSubsystem* GetOwningSubsystem() const { return OwningSubsystem; }

	// Declare method with forward-declared type
	void SetOwningSubsystem(UScenarioInstanceSubsystem* Subsystem);

	// Change method signature to accept subsystem during initialization
	void Initialize(UScenarioInstanceSubsystem* InOwningSubsystem);

protected:
	// Replicated array of scenario instances
	UPROPERTY(Replicated)
	TArray<UScenarioInstance*> ReplicatedInstances;
	
	virtual void PostInitializeComponents() override;
	
	// Reference to the owning subsystem
	UPROPERTY()
	TObjectPtr<UScenarioInstanceSubsystem> OwningSubsystem;

	friend class UScenarioInstanceSubsystem;
};
