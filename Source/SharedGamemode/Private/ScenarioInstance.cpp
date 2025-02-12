// Impact Forge LLC 2024


#include "ScenarioInstance.h"

#include "ScenarioInstanceSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Tasks/ScenarioObjective.h"
#include "Tasks/ScenarioTask_ObjectiveTracker.h"
#include "Tasks/ScenarioTask_StageService.h"

UScenarioInstance::UScenarioInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Initialize tag stacks container
	TagStacks.OnTagCountChanged.AddUObject(this, &ThisClass::OnTagStackChanged);
	ScenarioState = EScenarioState::None;
	PreviousStageResult = EScenarioResult::None;
}

void UScenarioInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UScenarioInstance, ScenarioAsset);
	DOREPLIFETIME(UScenarioInstance, ScenarioState);
	DOREPLIFETIME(UScenarioInstance, CurrentStage);
	DOREPLIFETIME(UScenarioInstance, PreviousStageResult);
	DOREPLIFETIME(UScenarioInstance, TagStacks);
	DOREPLIFETIME(UScenarioInstance, RuntimeTags);
}

UWorld* UScenarioInstance::GetWorld() const
{
	return UObject::GetWorld();
}

void UScenarioInstance::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
}

bool UScenarioInstance::InitScenario(UGameplayScenario* Scenario, const FGameplayTagContainer& InitTags)
{
	if (!ensure(IsValid(Scenario) && IsValid(Scenario->InitialStage)))
	{
		// Can't start without initial stage
		return false;
	}

	ScenarioAsset = Scenario;
	RuntimeTags.AppendTags(InitTags);
	ScenarioState = EScenarioState::Active;

	// Start with initial stage from scenario
	EnterStage(ScenarioAsset->InitialStage);

	return true;
}

void UScenarioInstance::EndScenario(bool bCancelled)
{
	// Update state
	ScenarioState = bCancelled ? EScenarioState::Cancelled : ScenarioState;

	// Clean up current stage
	if (IsValid(CurrentStage))
	{
		ExitStage(CurrentStage);
	}

	// Clean up global services
	for (auto* Service : GlobalServices)
	{
		if (IsValid(Service))
		{
			Service->EndPlay(bCancelled);
		}
	}
	GlobalServices.Empty();

	// Notify subscribers
	OnScenarioEnded.Broadcast(this, bCancelled);
}

TArray<UScenarioTask_ObjectiveTracker*> UScenarioInstance::GetCurrentObjectiveTrackers()
{
	return ObjectiveTrackers;
}

void UScenarioInstance::ForEachStageService(TFunctionRef<void(UScenarioTask_StageService*)> Func)
{
	// Process global services
	for (auto* Service : GlobalServices)
	{
		if (IsValid(Service))
		{
			Func(Service);
		}
	}

	// Process stage-specific services
	for (auto* Service : StageServices)
	{
		if (IsValid(Service))
		{
			Func(Service);
		}
	}
}

EScenarioResult UScenarioInstance::EvaluateObjectives()
{
    if (!IsValid(CurrentStage))
    {
        return EScenarioResult::None;
    }

    // Get completion mode for this stage
    EScenarioCompletionMode StageCompletionMode = CurrentStage->CompletionMode;

    // Track completion states for each objective
    TMap<UScenarioObjective*, EScenarioResult> ObjectiveStates;
    
    // Current overall stage state - starts as success for AllSuccess mode,
    // starts as failure for AnySuccess mode
    EScenarioResult StageState = EScenarioResult::InProgress;

    // Stage completion rules
    const bool bStageRequiresAllSuccess = StageCompletionMode == EScenarioCompletionMode::AllSuccess;
    const bool bStageNeedsAnySuccess = StageCompletionMode == EScenarioCompletionMode::AnySuccess;

    // Check all objective trackers
    for (UScenarioTask_ObjectiveTracker* Tracker : ObjectiveTrackers)
    {
        if (!IsValid(Tracker))
        {
            continue;
        }

        // Get current tracker state and objective rules
        EScenarioResult TrackerState = Tracker->GetTrackerState();
        UScenarioObjective* Objective = Tracker->GetObjective();
        const EScenarioCompletionMode ObjectiveMode = Objective->CompletionMode;

        // Quick failure/success checks
        if (bStageRequiresAllSuccess && 
            ObjectiveMode == EScenarioCompletionMode::AllSuccess && 
            TrackerState == EScenarioResult::Failure)
        {
            // If stage needs all successes and any required tracker fails,
            // the whole stage fails immediately
            return EScenarioResult::Failure;
        }

        if (bStageNeedsAnySuccess && 
            ObjectiveMode == EScenarioCompletionMode::AnySuccess && 
            TrackerState == EScenarioResult::Success)
        {
            // If stage needs any success and any tracker succeeds,
            // the whole stage succeeds immediately
            return EScenarioResult::Success;
        }

        // Track objective state
        EScenarioResult& ObjState = ObjectiveStates.FindOrAdd(
            Objective, 
            ObjectiveMode == EScenarioCompletionMode::AllSuccess ? 
                EScenarioResult::Success : 
                EScenarioResult::Failure
        );

        // Update objective state based on tracker
        if (ObjectiveMode == EScenarioCompletionMode::AllSuccess)
        {
            // For AllSuccess objectives, any failure or in-progress
            // tracker means the objective isn't complete
            if (TrackerState != EScenarioResult::Success)
            {
                ObjState = TrackerState == EScenarioResult::Failure ? 
                    EScenarioResult::Failure : 
                    EScenarioResult::InProgress;
            }
        }
        else // AnySuccess mode
        {
            // For AnySuccess objectives, any success means complete,
            // all failures means failed, otherwise still in progress
            if (TrackerState == EScenarioResult::Success)
            {
                ObjState = EScenarioResult::Success;
            }
            else if (TrackerState == EScenarioResult::InProgress)
            {
                ObjState = EScenarioResult::InProgress;
            }
        }
    }

    // Evaluate final stage state based on all objectives
    int32 SuccessCount = 0;
    int32 CompleteCount = 0;

    for (const auto& KV : ObjectiveStates)
    {
        if (KV.Value != EScenarioResult::InProgress)
        {
            CompleteCount++;
            if (KV.Value == EScenarioResult::Success)
            {
                SuccessCount++;
            }
        }
    }

    // All objectives must be complete to finish the stage
    if (CompleteCount == ObjectiveStates.Num())
    {
        // Stage success/failure depends on completion mode
        if (bStageRequiresAllSuccess)
        {
            // AllSuccess: every objective must succeed
            return SuccessCount == CompleteCount ? 
                EScenarioResult::Success : 
                EScenarioResult::Failure;
        }
        else
        {
            // AnySuccess: at least one objective must succeed
            return SuccessCount > 0 ? 
                EScenarioResult::Success : 
                EScenarioResult::Failure;
        }
    }

    // Still waiting on some objectives
    return EScenarioResult::InProgress;
}

bool UScenarioInstance::TryProgressStage()
{
	if (!HasAuthority() || !IsValid(CurrentStage))
	{
		return false;
	}

	// Evaluate objectives
	EScenarioResult StageResult = EvaluateObjectives();
	if (StageResult == EScenarioResult::InProgress)
	{
		return false;
	}

	// Handle stage transition
	float Delay = GetStageTransitionDelay();
	if (Delay > 0)
	{
		// Delayed transition
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(this, &ThisClass::ProgressStage_Internal, StageResult);
		GetWorld()->GetTimerManager().SetTimer(StageProgressionTimer, TimerDelegate, Delay, false);
	}
	else
	{
		// Immediate transition
		ProgressStage_Internal(StageResult);
	}

	return true;
}

void UScenarioInstance::AddTagStack(FGameplayTag Tag, int32 StackCount)
{
	if (Tag.IsValid() && StackCount > 0)
	{
		TagStacks.AddStack(Tag, StackCount);
	}
}

void UScenarioInstance::RemoveTagStack(FGameplayTag Tag, int32 StackCount)
{
	if (Tag.IsValid() && StackCount > 0)
	{
		TagStacks.RemoveStack(Tag, StackCount);
	}
}

int32 UScenarioInstance::GetTagStackCount(FGameplayTag Tag) const
{
	return TagStacks.GetStackCount(Tag);
}

bool UScenarioInstance::HasAuthority() const
{
	// Get the world this instance is in
	if (UWorld* World = GetWorld())
	{
		// Client builds never have authority
		if (World->GetNetMode() == NM_Client)
		{
			return false;
		}
		return true;
	}
	return false;
}

void UScenarioInstance::OnTagStackChanged(FGameplayTag Tag, int32 NewCount, int32 OldCount)
{
	/*if (UWorld* World = GetWorld())
	{
		if (UScenarioInstanceSubsystem* Subsystem = World->GetSubsystem<UScenarioInstanceSubsystem>())
		{
			FScenarioTagStackChanged EventData(this, Tag, NewCount, OldCount);
			Subsystem->BroadcastMessage(TAG_ScenarioTagStackChanged, EventData);
		}
	}*/
}

void UScenarioInstance::EnterStage(UScenarioStage* Stage)
{
	CurrentStage = Stage;

	if (HasAuthority())
	{
		// Create stage services
		for (auto* ServiceTemplate : Stage->StageServices)
		{
			if (auto* NewService = DuplicateObject<UScenarioTask_StageService>(ServiceTemplate, this))
			{
				StageServices.Add(NewService);
				NewService->BeginPlay();
			}
		}

		// Create objective trackers
		for (auto* Objective : Stage->Objectives)
		{
			for (auto* TrackerTemplate : Objective->ObjectiveTrackers)
			{
				if (auto* NewTracker = DuplicateObject<UScenarioTask_ObjectiveTracker>(TrackerTemplate, this))
				{
					ObjectiveTrackers.Add(NewTracker);
					NewTracker->BeginPlay();
				}
			}
		}
	}
}

void UScenarioInstance::ExitStage(UScenarioStage* Stage)
{
	// Clean up stage services
	for (auto* Service : StageServices)
	{
		if (IsValid(Service))
		{
			Service->EndPlay(false);
		}
	}
	StageServices.Empty();

	// Clean up objective trackers
	for (auto* Tracker : ObjectiveTrackers)
	{
		if (IsValid(Tracker))
		{
			Tracker->EndPlay(false);
		}
	}
	ObjectiveTrackers.Empty();
}

void UScenarioInstance::ProgressStage_Internal(EScenarioResult Transition)
{
	if (!IsValid(CurrentStage))
	{
		return;
	}

	// Determine next stage
	UScenarioStage* NextStage = Transition == EScenarioResult::Success ? 
		CurrentStage->NextStage_Success : CurrentStage->NextStage_Failure;

	// Exit current stage
	ExitStage(CurrentStage);
	PreviousStageResult = Transition;

	// Enter next stage or end scenario
	if (IsValid(NextStage))
	{
		EnterStage(NextStage);
	}
	else
	{
		ScenarioState = Transition == EScenarioResult::Success ? 
			EScenarioState::Success : EScenarioState::Failure;
		EndScenario(false);
	}
}

float UScenarioInstance::GetStageTransitionDelay() const
{
	float TotalDelay = 0.0f;

	// Get base delay from scenario template
	if (IsValid(ScenarioAsset))
	{
		TotalDelay += ScenarioAsset->BaseStageProgressionTimer;
	}

	// Add stage-specific delay
	if (IsValid(CurrentStage))
	{
		TotalDelay += CurrentStage->StageCompletionDelay;
	}

	return TotalDelay;
}

void UScenarioInstance::NotifyTaskUpdate(UScenarioTask_ObjectiveTracker* Task)
{
	if (ObjectiveTrackers.Contains(Task))
	{
		TryProgressStage();
	}
}
