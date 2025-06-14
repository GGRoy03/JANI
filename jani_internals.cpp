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
GenerateColor(void *Payload, void *User)
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

inline size_t
GenerateTextQuad(void *Payload, void *User)
{
    jani_quad         *Vertex = (jani_quad*)User;
    jani_text_payload *Quad   = (jani_text_payload*)Payload;
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
GenerateTextUV(void *Payload, void *User)
{
    jani_text_payload *Text   = (jani_text_payload*)Payload;
    jani_quad         *Vertex = (jani_quad*)User;
    f32               *Corner = nullptr;

    f32 u0 = (f32)Text->AtlasPosX / (f32)Text->TextureSizeX;
    f32 v0 = (f32)Text->AtlasPosY / (f32)Text->TextureSizeY;
    f32 u1 = (f32)(Text->AtlasPosX + Text->SizeX) / (f32)Text->TextureSizeX;
    f32 v1 = (f32)(Text->AtlasPosY + Text->SizeY) / (f32)Text->TextureSizeY;

    // Top left
    Corner    = (f32*)Vertex->TopLeft;
    Corner[0] = u0;
    Corner[1] = v1;

    // Top right
    Corner    = (f32*)Vertex->TopRight;
    Corner[0] = u1;
    Corner[1] = v1;

    // Bottom left
    Corner    = (f32*)Vertex->BottomLeft;
    Corner[0] = u0;
    Corner[1] = v0;

    // Bottom right
    Corner    = (f32*)Vertex->BottomRight;
    Corner[0] = u1;
    Corner[1] = v0;

    return 2 * sizeof(f32);
}

}
