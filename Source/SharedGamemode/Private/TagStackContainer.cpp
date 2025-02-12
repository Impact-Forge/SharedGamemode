// Impact Forge LLC 2024


#include "TagStackContainer.h"

FString FTagStack::GetDebugString() const
{
	return FString::Printf(TEXT("%s x%d"), *Tag.ToString(), StackCount);
}

FTagStackContainer::FTagStackContainer()
{
}

void FTagStackContainer::AddStack(FGameplayTag Tag, int32 StackCount)
{
	if (!Tag.IsValid())
	{
		// Early out if tag is invalid to prevent issues
		return;
	}

	// Only process positive stack additions
	if (StackCount > 0)
	{
		// Try to find existing stack
		for (FTagStack& Stack : Stacks)
		{
			if (Stack.Tag == Tag)
			{
				// Found existing stack - update it
				const int32 OldCount = Stack.StackCount;
				const int32 NewCount = OldCount + StackCount;
				Stack.StackCount = NewCount;
                
				// Update lookup map
				TagToCountMap[Tag] = NewCount;
                
				// Mark for replication
				MarkItemDirty(Stack);
                
				// Notify listeners
				OnTagCountChanged.Broadcast(Tag, NewCount, OldCount);
				return;
			}
		}

		// No existing stack found - create new one
		FTagStack& NewStack = Stacks.Emplace_GetRef(Tag, StackCount);
		MarkItemDirty(NewStack);
		TagToCountMap.Add(Tag, StackCount);
		OnTagCountChanged.Broadcast(Tag, StackCount, 0);
	}
}

void FTagStackContainer::RemoveStack(FGameplayTag Tag, int32 StackCount)
{
	if (!Tag.IsValid())
	{
		return;
	}

	if (StackCount > 0)
	{
		// Find and update existing stack
		for (auto It = Stacks.CreateIterator(); It; ++It)
		{
			FTagStack& Stack = *It;
			if (Stack.Tag == Tag)
			{
				const int32 OldCount = Stack.StackCount;
                
				if (Stack.StackCount <= StackCount)
				{
					// Removing all stacks - remove entry entirely
					It.RemoveCurrent();
					TagToCountMap.Remove(Tag);
					MarkArrayDirty();
					OnTagCountChanged.Broadcast(Tag, 0, OldCount);
				}
				else
				{
					// Partial removal - update count
					const int32 NewCount = Stack.StackCount - StackCount;
					Stack.StackCount = NewCount;
					TagToCountMap[Tag] = NewCount;
					MarkItemDirty(Stack);
					OnTagCountChanged.Broadcast(Tag, NewCount, OldCount);
				}
				return;
			}
		}
	}
}

void FTagStackContainer::SetStack(FGameplayTag Tag, int32 StackCount)
{
	if (!Tag.IsValid())
	{
		return;
	}

	if (StackCount > 0)
	{
		// Try to find existing stack
		for (FTagStack& Stack : Stacks)
		{
			if (Stack.Tag == Tag)
			{
				// Update existing stack
				const int32 OldCount = Stack.StackCount;
				Stack.StackCount = StackCount;
				TagToCountMap[Tag] = StackCount;
				MarkItemDirty(Stack);
				OnTagCountChanged.Broadcast(Tag, StackCount, OldCount);
				return;
			}
		}

		// Create new stack
		FTagStack& NewStack = Stacks.Emplace_GetRef(Tag, StackCount);
		MarkItemDirty(NewStack);
		TagToCountMap.Add(Tag, StackCount);
		OnTagCountChanged.Broadcast(Tag, StackCount, 0);
	}
}

void FTagStackContainer::ClearStack(FGameplayTag Tag)
{
	if (!Tag.IsValid())
	{
		return;
	}

	// Find and remove the stack
	for (auto It = Stacks.CreateIterator(); It; ++It)
	{
		FTagStack& Stack = *It;
		if (Stack.Tag == Tag)
		{
			const int32 OldCount = Stack.StackCount;
			It.RemoveCurrent();
			TagToCountMap.Remove(Tag);
			MarkArrayDirty();
			OnTagCountChanged.Broadcast(Tag, 0, OldCount);
			return;
		}
	}
}

void FTagStackContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		const FGameplayTag Tag = Stacks[Index].Tag;
		const int32 OldCount = Stacks[Index].StackCount;
		TagToCountMap.Remove(Tag);
		OnTagCountChanged.Broadcast(Tag, 0, OldCount);
	}
}

void FTagStackContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FTagStack& Stack = Stacks[Index];
		TagToCountMap.Add(Stack.Tag, Stack.StackCount);
		OnTagCountChanged.Broadcast(Stack.Tag, Stack.StackCount, 0);
	}
}

void FTagStackContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	    for (int32 Index : ChangedIndices)
    {
        const FTagStack& Stack = Stacks[Index];
        const int32 OldCount = TagToCountMap.FindRef(Stack.Tag);
        TagToCountMap[Stack.Tag] = Stack.StackCount;
        OnTagCountChanged.Broadcast(Stack.Tag, Stack.StackCount, OldCount);
    }
}

void FTagStackContainer::RebuildTagToCountMap()
{
	TagToCountMap.Empty(Stacks.Num());
	for (const FTagStack& Stack : Stacks)
	{
		TagToCountMap.Add(Stack.Tag, Stack.StackCount);
	}
}
