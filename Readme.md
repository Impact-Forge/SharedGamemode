# Shared Gamemode Plugin

An Unreal Engine Game Feature Plugin for modular gameplay scenarios with enhanced voting and persistence

## Overview

The Shared Gamemode plugin provides a robust system for creating and managing gameplay scenarios in Unreal Engine. It allows developers to construct complex gameplay experiences by composing multiple scenarios, each containing various gameplay elements such as maps, components, and gameplay abilities.

## Dependencies

- Requires [GameplayEventRouter](https://github.com/EmpiresCommunity/GameplayEventRouter)
- Unreal Engine 4.27+ or UE5
- Place in Plugins/GameFeatures/ folder

## Required Plugins

```json
{
  "Plugins": [
    {
      "Name": "GameplayAbilities",
      "Enabled": true
    },
    {
      "Name": "ModularGameplay",
      "Enabled": true
    },
    {
      "Name": "GameFeatures",
      "Enabled": true
    }
  ]
}
```

## Core Components

### 1. ScenarioTransitionComponent

The base component for handling map/scenario voting and transitions. Key features:

- Networked voting system for scenario selection
- Configurable voting duration and number of options
- Gameplay tag filtering for scenario selection
- Vote tracking and replication
- Automatic scenario transition on vote completion

Usage example:

```cpp
// Start a vote
TransitionComponent->StartVoting();

// Cast a vote (from client)
TransitionComponent->ServerCastVote(ScenarioId);
```

### 2. EnhancedScenarioTransitionComponent

Extends the base transition component with additional features:

- Performance-based vote weighting
- Veto system for scenarios
- Weighted voting based on player performance
- Player vote weight tracking
- Scenario rotation rules integration

Configuration options:

```cpp
UPROPERTY(EditDefaultsOnly)
float MinimumVoteWeight = 0.5f;

UPROPERTY(EditDefaultsOnly)
float MaximumVoteWeight = 2.0f;

UPROPERTY(EditDefaultsOnly)
float PerformanceWeightMultiplier = 0.1f;
```

### 3. ScenarioPersistenceManager

Game instance subsystem that handles persistent scenario data:

- Tracks scenario play statistics
- Manages scenario rotation rules
- Stores player counts and popularity
- JSON-based persistence
- Weighted scenario selection based on history

## Gameplay Scenarios

A Gameplay Scenario is a data asset that defines a complete gameplay experience. Each scenario can:

- Load specific maps
- Add components to actors
- Stream level instances
- Apply GAS (Gameplay Ability System) primitives
- Activate or deactivate other scenarios

### Scenario Composition Example

```
SC_BaenIslandCQDay (in GameFeatures/Scenarios/BaenIslandConquest)
  Actions: [
    ActivateScenario: SC_BaenIslandDay
    ActivateScenario: SC_Conquest
    StreamLevelInstance: [ L_BaenIslandCQ ]
  ]

SC_BaenIslandDay (in GameFeatures/Terrain/BaenIslandConquest)
  Actions: [
    ChangeMap: L_BaenIsland_Terrain
    StreamLevelInstance: [ L_BaenIsland_Atmo_Day ]
  ]

SC_Conquest (in GameFeatures/Gamemode)
  Actions: [
    ApplyGASPrimitives: {To: TeamState, AddAttributeSets: [Tickets], AddAbilities: [BP_SubtractTicketOnDeath_GA, BP_ConquestVictory_GA] }
  ]
```

## Enhanced Voting System

### Performance-Based Voting

- Player votes are weighted based on performance
- Configurable weight ranges and multipliers
- Fair representation while rewarding skilled players

### Map Rotation

- Tracks scenario play history
- Ensures map variety
- Prevents repetitive scenario selection

### Veto System

- Players can veto undesired scenarios
- Configurable veto thresholds
- Prevents consistently unpopular scenarios

## Persistence System

Scenario statistics and rotation settings are automatically saved to:

```
ProjectSavedDir/ScenarioStats.json
```

The persistence system tracks:

- Times played
- Total votes received
- Average player count
- Last played timestamp
- Rotation weights and rules

## Setup and Implementation

1. Add the plugin to your project's Plugins folder
2. Enable it in your project settings
3. Add components to your GameState:

```cpp
UPROPERTY()
UEnhancedScenarioTransitionComponent* ScenarioTransition;

// In cpp:
ScenarioTransition = CreateDefaultSubobject<UEnhancedScenarioTransitionComponent>("ScenarioTransition");
```

4. Configure voting settings:

```cpp
ScenarioTransition->VotingDuration = 30.0f;    // 30 seconds voting time
ScenarioTransition->NumScenarioOptions = 3;     // Show 3 options
ScenarioTransition->MinimumVoteWeight = 0.5f;   // Minimum vote weight
ScenarioTransition->MaximumVoteWeight = 2.0f;   // Maximum vote weight
```

### Integration with Game Phases

The system can automatically trigger voting based on game phases using the ScenarioGameModeComponent:

```cpp
// Configure in Blueprint or C++
ScenarioGameModeComponent->EndGamePhaseTag = FGameplayTag::RequestGameplayTag("Game.EndGame");
```

## Asset Registry Integration

Scenarios are exposed to the Asset Registry with three tags:

- `Name`
- `Description`
- `bTopLevel`

### Querying Scenarios

```cpp
UFUNCTION(BlueprintPure, Category = "Assets")
static bool GetAssetTagAsString(FPrimaryAssetId Asset, FName Tag, FString& OutString);

UFUNCTION(BlueprintPure, Category = "Assets")
static bool GetAssetTagAsText(FPrimaryAssetId Asset, FName Tag, FText& OutText);

UFUNCTION(BlueprintPure, Category = "Assets")
static bool GetAssetTagAsBool(FPrimaryAssetId Asset, FName Tag, bool& OutBool);
```

### Activating Scenarios

From Console:

```
StartScenario ScenarioAssetId
```

From C++:

```cpp
UScenarioInstanceSubsystem* ScenarioSystem = GetGameInstance()->GetSubsystem<UScenarioInstanceSubsystem>();
ScenarioSystem->SetPendingScenario(YourScenario);
ScenarioSystem->TransitionToPendingScenario();
```

## Best Practices

1. Scenario Organization:

   - Keep scenarios modular and focused
   - Use composition to build complex experiences
   - Separate terrain, gameplay, and configuration scenarios

2. Performance Considerations:

   - Be mindful of level streaming operations
   - Manage component lifecycle properly
   - Use scenario deactivation to clean up resources

3. Networking:
   - Scenarios are replicated automatically
   - Server controls scenario activation/deactivation
   - Clients receive and apply scenario changes automatically

## Common Issues and Solutions

1. Scenario Not Loading:

   - Verify asset references are valid
   - Check for circular dependencies
   - Ensure required plugins are enabled

2. Network Synchronization:
   - Scenarios must be activated on the server
   - Verify GamestateScenarioComponent is properly replicated
   - Check network role before performing operations

## Future Development

- UE5 support and optimization
- Enhanced GAS integration
- Dynamic scenario activation/deactivation
- Improved map rotation algorithms

## License

Licensed under the Apache License, Version 2.0
