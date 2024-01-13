#include "Jogo.h"
#include "Arena.h"

void Arena::ReleaseMemory()
{
	Jogo::Free(BaseAddress);
}

Arena Arena::CreateArena(size_t ArenaSize)
{
	Arena NewArena = { ArenaSize };

	NewArena.BaseAddress = (u8*)Jogo::Allocate(ArenaSize);
	NewArena.CurrentLocation = NewArena.BaseAddress;

	return NewArena;
}
