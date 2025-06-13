#include "jani.h"

namespace JANI
{

// NOTE: What is the point of this? Can't we already generate those?
// Alright, so how do we generate the vertices given their position on screen.
// Well, what is the payload like?
inline jani_vertex_output
GenerateQuadVertex(jani_context *Context, void *Payload, void *User)
{
    Jani_Unused(User);

    jani_quad_payload *Quad = (jani_quad_payload*)Payload;

    u32    VertexCount = 4;
    size_t DataSize    = VertexCount * 2 * sizeof(f32);
    f32   *Vertices    = (f32*)Context->Allocator.Allocate(DataSize);

    // Top left
    Vertices[0] = Quad->TopLeftX;
    Vertices[1] = Quad->TopLeftY;

    // Top right
    Vertices[2] = Quad->TopLeftX + Quad->SizeX;
    Vertices[3] = Quad->TopLeftY;

    // Bottom left
    Vertices[4] = Quad->TopLeftX;
    Vertices[5] = Quad->TopLeftY - Quad->SizeY;

    // Bottom right
    Vertices[6] = Quad->TopLeftX + Quad->SizeX;
    Vertices[7] = Quad->TopLeftY - Quad->SizeY;

    jani_vertex_output Output = {};
    Output.Data   = Vertices;
    Output.Size   = DataSize;
    Output.Stride = 2 * sizeof(f32);

    return Output;
}

inline jani_vertex_output
GenerateQuadColor(jani_context *Context, void *Payload, void *User)
{
    Jani_Unused(User);
    Jani_Unused(Payload);

    jani_vertex_output Output = {};

    u32 VertexCount = 4;

    size_t DataSize = VertexCount * 3 * sizeof(f32);
    f32   *Colors   = (f32*)Context->Allocator.Allocate(DataSize);

    Colors[0] = 1.0f; Colors[1]  = 0.0f; Colors[2]  = 0.0f; 
    Colors[3] = 0.0f; Colors[4]  = 1.0f; Colors[5]  = 0.0f; 
    Colors[6] = 0.0f; Colors[7]  = 0.0f; Colors[8]  = 1.0f; 
    Colors[9] = 1.0f; Colors[10] = 1.0f; Colors[11] = 1.0f; 

    Output.Data   = Colors;
    Output.Size   = DataSize;
    Output.Stride = 3 * sizeof(f32);

    return Output;
}

inline jani_vertex_output
GenerateTextUV(jani_context *Context, void *Payload, void *User)
{
    Jani_Unused(User);
    Jani_Unused(Payload);
    Jani_Unused(Context);

    jani_vertex_output Output = {};

    return Output;
}

inline jani_vertex_output
GenerateQuadIndex(jani_context *Context, void *Payload, void *User) 
{
    Jani_Unused(User);
    Jani_Unused(Payload);

    jani_vertex_output Output = {};

    u32 IndexCount = 6;

    size_t DataSize = IndexCount * sizeof(u32);
    u32   *Indices  = (u32*)Context->Allocator.Allocate(DataSize);

    Indices[0] = 0; Indices[1] = 1; Indices[2] = 3;
    Indices[3] = 3; Indices[4] = 2; Indices[5] = 0;

    Output.Data = Indices;
    Output.Size = DataSize;

    return Output;
}

}
