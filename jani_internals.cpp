#include "jani.h"

namespace JANI
{

// NOTE: Thinking of adding the quad to the payload?

inline size_t
GenerateQuadVertex(void *Payload, void *User)
{
    jani_quad         *Vertex = (jani_quad*)User;
    jani_quad_payload *Quad   = (jani_quad_payload*)Payload;
    f32               *Corner = nullptr;

    Corner    = (f32*)Vertex->TopLeft;
    Corner[0] = Quad->TopLeftX;
    Corner[1] = Quad->TopLeftY;

    Corner    = (f32*)Vertex->TopRight;
    Corner[0] = Quad->TopLeftX + Quad->SizeX;
    Corner[1] = Quad->TopLeftY;

    Corner    = (f32*)Vertex->BottomLeft;
    Corner[0] = Quad->TopLeftX;
    Corner[1] = Quad->TopLeftY - Quad->SizeY;

    Corner    = (f32*)Vertex->BottomRight;
    Corner[0] = Quad->TopLeftX + Quad->SizeX;
    Corner[1] = Quad->TopLeftY - Quad->SizeY;

    return 2 * sizeof(f32);
}

inline size_t 
GenerateQuadColor(void *Payload, void *User)
{
    Jani_Unused(Payload);

    jani_quad *Vertex = (jani_quad*)User;
    f32       *Corner = nullptr;

    Corner = (f32*)Vertex->TopLeft;
    Corner[0] = 1.0f;
    Corner[1] = 0.0f;
    Corner[2] = 0.0f;

    Corner    = (f32*)Vertex->TopRight;
    Corner[0] = 0.0f;
    Corner[1] = 1.0f;
    Corner[2] = 0.0f;

    Corner    = (f32*)Vertex->BottomLeft;
    Corner[0] = 0.0f;
    Corner[1] = 0.0f;
    Corner[2] = 1.0f;

    Corner    = (f32*)Vertex->BottomRight;
    Corner[0] = 1.0f;
    Corner[1] = 1.0f;
    Corner[2] = 1.0f;

    return 3 * sizeof(f32);
}

}
