#pragma once

#include "jani_types.h"
#include <cstdlib> // Malloc
#include <cstring> // memcpy - memset


#define Jani_Assert(cond) do { if (!(cond)) __debugbreak(); } while (0)

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

    inline void PushNoCheck(T Value)
    {
        Values[Size] = Value;
        Size        += 1;
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

    jani_allocator()
            : PoolOrder(0)
            , MinBlockOrder(JANI_ALLOCATOR_MIN_BLOCK_ORDER)
            , Memory(nullptr)
            , Capacity(0)
            , FreeList(nullptr)
       {}

    jani_allocator(size_t Size)
    {
        u8     PO           = 0;
        size_t MaxBlockSize = 1llu << JANI_ALLOCATOR_MIN_BLOCK_ORDER;
        while ((MaxBlockSize << 1) <= Size)
        {
            MaxBlockSize <<= 1;
            PO++;
        }

        MinBlockOrder = JANI_ALLOCATOR_MIN_BLOCK_ORDER;
        PoolOrder     = PO;
        Memory        = (u8*)malloc(Size);
        Capacity      = Size;
        FreeList      = (jani_allocator_block**)malloc(sizeof(jani_allocator_block*) * (PO + 1));

        jani_allocator_block* Initial = (jani_allocator_block*)Memory;
        Initial->Next = NULL;

        for (u8 Index = 0; Index < PoolOrder; Index++)
        {
            FreeList[Index] = NULL;
        }
        FreeList[PoolOrder] = Initial;
    }

    u32 SizeToOrder(size_t Size)
    {
        size_t Needed = (Size + ((1ULL << MinBlockOrder) - 1)) >> MinBlockOrder;

        u32 Order = 0;
        while ((1ULL << Order) < Needed)
        {
            Order++;
        }

        return Order;
    }

    void SplitBlock(uint32_t Order)
    {
        jani_allocator_block* Block = FreeList[Order];
        Jani_Assert(Block);
        FreeList[Order] = Block->Next;

        size_t HalfSize = (1ULL << (Order + MinBlockOrder - 1));
        jani_allocator_block* Buddy = (jani_allocator_block*)((uint8_t*)Block + HalfSize);

        Block->Next = Buddy;
        Buddy->Next = FreeList[Order - 1];
        FreeList[Order - 1] = Block;
    }

    jani_allocator_block* GetBuddy(jani_allocator_block* Block, uint32_t Order)
    {
        size_t Offset    = (uint8_t*)Block - Memory;
        size_t BlockSize = (1ULL << (Order + MinBlockOrder));
        size_t BuddyOff  = Offset ^ BlockSize;
        return (jani_allocator_block*)(Memory + BuddyOff);
    }

    void* Allocate(size_t Size)
    {
        uint32_t Order = SizeToOrder(Size);
        if (Order > PoolOrder) return NULL;

        uint32_t EmptyOrder = Order;
        while (EmptyOrder <= PoolOrder && FreeList[EmptyOrder] == NULL)
        {
            EmptyOrder++;
        }

        if (EmptyOrder > PoolOrder) return NULL;

        while (EmptyOrder > Order)
        {
            SplitBlock(EmptyOrder);
            EmptyOrder--;
        }

        jani_allocator_block* Block = FreeList[Order];
        FreeList[Order]              = Block->Next;

        memset(Block, 0, Size);
        return Block;
    }

    void Free(void* Pointer, size_t Size)
    {
        Jani_Assert(Pointer && Memory);

        uint32_t Order = SizeToOrder(Size);
        jani_allocator_block* Block = (jani_allocator_block*)Pointer;

        while (Order < PoolOrder)
        {
            jani_allocator_block* Buddy = GetBuddy(Block, Order);

            jani_allocator_block** PrevPtr = &FreeList[Order];
            jani_allocator_block*  Current = *PrevPtr;
            while (Current && Current != Buddy)
            {
                PrevPtr = &Current->Next;
                Current = Current->Next;
            }

            if (!Current) break;

            *PrevPtr = Buddy->Next;

            if (Buddy < Block)
            {
                Block = Buddy;
            }
            Order++;
        }

        Block->Next     = FreeList[Order];
        FreeList[Order] = Block;
    }
};

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
