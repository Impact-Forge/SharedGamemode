// Impact Forge LLC 2024

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "TagStackContainer.generated.h"

// Represents a single tag and its stack count
USTRUCT()
struct FTagStack : public FFastArraySerializerItem
{
    GENERATED_BODY()

    FTagStack() {}

    FTagStack(FGameplayTag InTag, int32 InStackCount)
        : Tag(InTag)
        , StackCount(InStackCount)
    {}

    // The gameplay tag being tracked
    UPROPERTY()
    FGameplayTag Tag;

    // How many stacks of this tag exist
    UPROPERTY()
    int32 StackCount = 0;

    // Debug helper
    FString GetDebugString() const;
};

// Delegate to notify when tag counts change
DECLARE_MULTICAST_DELEGATE_ThreeParams(FTagStackChanged, FGameplayTag /*Tag*/, int32 /*NewCount*/, int32 /*OldCount*/);

// Container that manages tag stacks with network replication support
USTRUCT()
struct FTagStackContainer : public FFastArraySerializer
{
    GENERATED_BODY()

    FTagStackContainer();

    // Core stack manipulation
    void AddStack(FGameplayTag Tag, int32 StackCount);
    void RemoveStack(FGameplayTag Tag, int32 StackCount);
    void SetStack(FGameplayTag Tag, int32 StackCount);
    void ClearStack(FGameplayTag Tag);

    // Query methods
    int32 GetStackCount(FGameplayTag Tag) const { return TagToCountMap.FindRef(Tag); }
    bool ContainsTag(FGameplayTag Tag) const { return TagToCountMap.Contains(Tag); }

    // Network serialization support
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FTagStack, FTagStackContainer>(Stacks, DeltaParms, *this);
    }

    // Replication callbacks
    void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
    void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

    // Change notification
    FTagStackChanged OnTagCountChanged;

private:
    // The actual storage of tag stacks
    UPROPERTY()
    TArray<FTagStack> Stacks;

    // Quick lookup map for stack counts
    TMap<FGameplayTag, int32> TagToCountMap;

    // Helper to rebuild the lookup map
    void RebuildTagToCountMap();
};

// Enable network delta serialization
template<>
struct TStructOpsTypeTraits<FTagStackContainer> : public TStructOpsTypeTraitsBase2<FTagStackContainer>
{
    enum
    {
        WithNetDeltaSerializer = true,
    };
};