#include "jani.h"

namespace JANI
{

inline jani_vertex_output
GenerateQuadVertex(jani_context *Context, void *Payload, void *User)
{
    Jani_Unused(User);
    Jani_Unused(Payload);

    jani_vertex_output Output = {};

    u32 VertexCount = 4;

    size_t DataSize = VertexCount * 2 * sizeof(f32);
    f32   *Vertices = (f32*)Context->Allocator.Allocate(DataSize);

    Vertices[0] = -0.25f; Vertices[1] =  0.25f;
    Vertices[2] =  0.25f; Vertices[3] =  0.25f;
    Vertices[4] = -0.25f; Vertices[5] = -0.25f;
    Vertices[6] =  0.25f; Vertices[7] = -0.25f;

    Output.Data   = Vertices;
    Output.Size   = DataSize;
    Output.Offset = 0;
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
    Colors[3] = 1.0f; Colors[4]  = 0.0f; Colors[5]  = 0.0f; 
    Colors[6] = 1.0f; Colors[7]  = 0.0f; Colors[8]  = 0.0f; 
    Colors[9] = 1.0f; Colors[10] = 0.0f; Colors[11] = 0.0f; 

    Output.Data   = Colors;
    Output.Size   = DataSize;
    Output.Offset = 0;
    Output.Stride = 3 * sizeof(f32);

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
