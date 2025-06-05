#pragma once

#include "jani_types.h"
#include <cstdlib> // Malloc
#include <cstring> // memcpy - memset

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
	    u32 NewCapacity = 8;
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
