#pragma once
#include "int_types.h"

struct Arena
{
	size_t Size;
	u8* BaseAddress;
	u8* CurrentLocation;

	void* Allocate(size_t Request)
	{
		u8* TopLimit = BaseAddress + Size;
		if (CurrentLocation + Request <= TopLimit)
		{
			u8* ReturnValue = CurrentLocation;
			CurrentLocation += (Request + 7) & ~7;		// round up to multiple of 8
			return ReturnValue;
		}
		return nullptr;
	}

	void ReleaseMemory();
	static Arena CreateArena(size_t ArenaSize);
};