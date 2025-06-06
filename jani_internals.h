#pragma once

#include "jani_types.h"
#include <cstdlib> // Malloc
#include <cstring> // memcpy - memset


namespace JANI
{

template <typename T>
struct JaniBumper
{
    u32    Size;
    size_t Capacity;
    T*     Values;

    JaniBumper()
    {
        Size     = 0;
        Capacity = 0;
        Values   = nullptr;
    }

    JaniBumper(size_t AllocSize)
    {
        Capacity = AllocSize;
        Values   = (T*)malloc(AllocSize);
    }

    inline void Reset()
    {
        Size = 0;
    }

    inline T& operator[](i32 Index)
    {
	ASSERT(Index >= 0 && Index < Size, "INTERNAL_JANI_FAILURE: FAILED TO INDEX INTO VECTOR");
	return Values[Index];
    }

    inline T& Last()
    {
	ASSERT(Size > 0, "INTERNAL_JANI_FAILURE: FAILED TO GET THE LAST ELEMENT OF VECTOR");
	return Values[Size -1];
    }

    inline void Push(T Value)
    {
	if(Size == Capacity)
	{
	    size_t NewCapacity = 8;
	    if(Size)
	    {
		NewCapacity  = Capacity * 2; // NOTE: Using a growth of 2 for now.
	    }

	    T* New = (T*)malloc(NewCapacity * sizeof(T));
	    if(New)
	    {
		memcpy(New, Values, Size * sizeof(T));
	    }
	    free(Values);

	    Values   = New;
	    Capacity = NewCapacity;
	}


	Values[Size] = Value;
	Size++;
    }

    inline void Pop()
    {
	ASSERT(Size > 0, "INTERNAL_JANI_FAILURE: FAILED TO POP OFF OF VECTOR");
	Size--;
    }
};

#ifndef JANI_ALLOCATOR_MIN_BLOCK_ORDER
#define JANI_ALLOCATOR_MIN_BLOCK_ORDER 5
#endif

struct jani_allocator_block 
{
    jani_allocator_block* Next;
};

struct jani_allocator
{
    uint8_t                PoolOrder;
    uint8_t                MinBlockOrder;
    uint8_t*               Memory;
    size_t                 Capacity;
    jani_allocator_block** FreeList;
};

static uint32_t
SizeToOrder(jani_allocator* Allocator, size_t Size)
{
    size_t Needed = (Size + ((1ULL << Allocator->MinBlockOrder) - 1))
                    >> Allocator->MinBlockOrder;

    uint32_t Order = 0;
    while ((1ULL << Order) < Needed)
    {
        Order++;
    }
    return Order;
}

static void
SplitBlock(jani_allocator* Allocator, uint32_t Order)
{
    jani_allocator_block* Block = Allocator->FreeList[Order];
    Assert(Block);
    Allocator->FreeList[Order] = Block->Next;

    size_t HalfSize = (1ULL << (Order + Allocator->MinBlockOrder - 1));
    jani_allocator_block* Buddy = (jani_allocator_block*)((uint8_t*)Block + HalfSize );

    Block->Next = Buddy;
    Buddy->Next = Allocator->FreeList[Order - 1];
    Allocator->FreeList[Order - 1] = Block;
}

static jani_allocator_block*
GetBuddy(jani_allocator* Allocator, jani_allocator_block* Block, uint32_t Order)
{
    size_t Offset    = (uint8_t*)Block - Allocator->Memory;
    size_t BlockSize = (1ULL << (Order + Allocator->MinBlockOrder));
    size_t BuddyOff  = Offset ^ BlockSize;
    return (jani_allocator_block*)(Allocator->Memory + BuddyOff);
}

jani_allocator
InitializeAllocator(size_t Size)
{
    jani_allocator Transient = { 0 };

    u8 PoolOrder = 0;
    size_t MaxBlockSize = 1llu << MIN_BLOCK_ORDER;
    while ((MaxBlockSize << 1) <= Size)
    {
        MaxBlockSize <<= 1;
        PoolOrder++;
    }

    Transient.MinBlockOrder = JANI_ALLOCATOR_MIN_BLOCK_ORDER;
    Transient.PoolOrder     = PoolOrder;
    Transient.Memory        = (u8*)malloc(Size);
    Transient.FreeList      =
	(jani_allocator_block**)malloc(sizeof(jani_allocator_block*)*(PoolOrder+1));
    Transient.Capacity      = Size;

    jani_allocator_block* Initial = (jani_allocator_block*)Transient.Memory;
    Initial->Next = NULL;

    for (u8 Index = 0; Index < PoolOrder; Index++)
    {
        Transient.FreeList[Index] = NULL;
    }
    Transient.FreeList[PoolOrder] = Initial;

    return Transient;
}

void*
AllocateMemory(jani_allocator* Allocator, size_t Size)
{
    uint32_t Order = SizeToOrder(Allocator, Size);
    if (Order > Allocator->PoolOrder) return NULL;

    uint32_t EmptyOrder = Order;
    while (EmptyOrder <= Allocator->PoolOrder &&
           Allocator->FreeList[EmptyOrder] == NULL) 
    {
        EmptyOrder++;
    }
    if (EmptyOrder > Allocator->PoolOrder) return NULL;

    while (EmptyOrder > Order) 
    {
        SplitBlock(Allocator, EmptyOrder);
        EmptyOrder--;
    }

    jani_allocator_block* Block     = Allocator->FreeList[Order];
    Allocator->FreeList[Order] = Block->Next;

    memset(Block, 0, Size);
    return Block;
}

void
FreeMemory(jani_allocator* Allocator, void* Pointer, size_t Size)
{
    Assert(Pointer && Allocator->Memory);

    uint32_t Order              = SizeToOrder(Allocator, Size);
    jani_allocator_block* Block = (jani_allocator_block*)Pointer;

    while (Order < Allocator->PoolOrder) 
    {
        jani_allocator_block* Buddy = GetBuddy(Allocator, Block, Order);

        jani_allocator_block** PrevPtr = &Allocator->FreeList[Order];
        jani_allocator_block*  Current = *PrevPtr;
        while (Current && Current != Buddy) {
            PrevPtr = &Current->Next;
            Current =  Current->Next;
        }

        if (!Current) break;

        *PrevPtr = Buddy->Next;

        if (Buddy < Block) 
        {
            Block = Buddy;
        }
        Order++;
    }

    Block->Next                = Allocator->FreeList[Order];
    Allocator->FreeList[Order] = Block;
}

// -------------------------
// DRAWING 
// -------------------------

struct JANI_VEC_2
{
    f32 x;
    f32 y;
};

struct jani_rect
{
    JANI_VEC_2 Pos;
    JANI_VEC_2 Size;

    inline bool operator==(jani_rect o)
    {
        return Pos.x == o.Pos.x   && Pos.y == o.Pos.y &&
        Size.x == o.Size.x && Size.y == o.Size.y;
    }
};

} // JANI Namespace
