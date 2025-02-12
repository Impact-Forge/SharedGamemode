// Impact Forge LLC 2024


#include "ScenarioReplicationProxy.h"

#include "Net/UnrealNetwork.h"

AScenarioReplicationProxy::AScenarioReplicationProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false);
}

void AScenarioReplicationProxy::AddReplicatedInstance(UScenarioInstance* Instance)
{
	if (!ReplicatedInstances.Contains(Instance))
	{
		ReplicatedInstances.Add(Instance);
	}
}

void AScenarioReplicationProxy::RemoveReplicatedInstance(UScenarioInstance* Instance)
{
	ReplicatedInstances.Remove(Instance);
}

void AScenarioReplicationProxy::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
}

void AScenarioReplicationProxy::SetOwningSubsystem(UScenarioInstanceSubsystem* Subsystem)
{
	OwningSubsystem = Subsystem;
}

void AScenarioReplicationProxy::Initialize(UScenarioInstanceSubsystem* InOwningSubsystem)
{
	OwningSubsystem = InOwningSubsystem;
}

void AScenarioReplicationProxy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AScenarioReplicationProxy, ReplicatedInstances);
}
