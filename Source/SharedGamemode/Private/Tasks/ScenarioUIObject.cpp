// Impact Forge LLC 2024


#include "Tasks/ScenarioUIObject.h"
#include "Tasks/ScenarioStage.h"

UScenarioStage* UScenarioUIObject::GetStage() const
{
	return Cast<UScenarioStage>(GetOuter());
}
