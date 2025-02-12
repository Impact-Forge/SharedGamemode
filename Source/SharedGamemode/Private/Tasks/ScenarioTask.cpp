// Impact Forge LLC 2024


#include "Tasks/ScenarioTask.h"

#include "ScenarioInstance.h"
#include "ScenarioTypes.h"
#include "Tasks/ScenarioTask_ObjectiveTracker.h"

UScenarioTask::UScenarioTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentResult = EScenarioResult::InProgress;
}

UWorld* UScenarioTask::GetWorld() const
{
	if (UScenarioInstance* Instance = GetScenarioInstance())
	{
		return Instance->GetWorld();
	}
	return nullptr;
}

UScenarioInstance* UScenarioTask::GetScenarioInstance() const
{
	return Cast<UScenarioInstance>(GetOuter());
}

void UScenarioTask::SetTaskResult(EScenarioResult NewResult)
{
	if (CurrentResult != NewResult)
	{
		CurrentResult = NewResult;
        
		if (UScenarioInstance* Instance = GetScenarioInstance())
		{
			Instance->NotifyTaskUpdate(Cast<UScenarioTask_ObjectiveTracker>(this));
		}
	}
}