#include "jani_opengl.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

namespace JANI
{


// WARN: This always uses GL_DYNAMIC_DRAW as a flag which is wrong.
void 
PrepareDrawCommands(jani_context *Context, jani_pipeline_state *State)
{
    jani_backend_draw_list *List = &State->DrawList;

    if(List->VtxBuffer.FrameSize > List->VtxBuffer.Size)
    {
        Context->Allocator.Free(List->VtxBuffer.Cpu, List->VtxBuffer.Size);

        List->VtxBuffer.Size = List->VtxBuffer.FrameSize + Kilobytes(5);
        glNamedBufferData(List->VtxBuffer.Buffer, List->VtxBuffer.Size,
                          nullptr, GL_DYNAMIC_DRAW);

        List->VtxBuffer.Cpu = (u8*)Context->Allocator.Allocate(List->VtxBuffer.Size);
    }

    if(List->IdxBuffer.FrameSize > List->IdxBuffer.Size)
    {
        List->IdxBuffer.Size = List->IdxBuffer.FrameSize + Kilobytes(5);
        glNamedBufferData(List->IdxBuffer.Buffer, List->IdxBuffer.Size,
                          nullptr, GL_DYNAMIC_DRAW);
    }

    for(u32 Index = 0; Index < List->DrawInfos.Size; Index++)
    {
        jani_draw_info Info = List->DrawInfos[Index];

        switch(Info.DrawType)
        {

        case JANI_DRAW_MESH:
        {
        } break;

        case JANI_DRAW_INSTANCED_MESH:
        {
        } break;

        // NOTE: This just works? We removed all of the weird allocations.
        case JANI_DRAW_QUAD:
        {
            jani_quad Vertex   = {};
            Vertex.TopLeft     = List->VtxBuffer.Cpu + List->VtxBuffer.WriteOffset;
            Vertex.BottomLeft  = Vertex.TopLeft      + State->InputStride;
            Vertex.TopRight    = Vertex.BottomLeft   + State->InputStride;
            Vertex.BottomRight = Vertex.TopRight     + State->InputStride;

            for(u32 Output = 0; Output < State->InputCount; Output++)
            {
                size_t Stride = State->Generators[Output](&Info.Payload.Quad, &Vertex);

                Vertex.TopLeft     += Stride;
                Vertex.TopRight    += Stride;
                Vertex.BottomLeft  += Stride;
                Vertex.BottomRight += Stride;
            }

            List->VtxBuffer.WriteOffset += 4 * State->InputStride;

            if(List->IdxBuffer.Buffer)
            {
                void *Write = List->IdxBuffer.Cpu + List->IdxBuffer.WriteOffset;

                u32 Indices[6] = 
                {
                    0 + List->IdxBuffer.BaseVertex,
                    1 + List->IdxBuffer.BaseVertex,
                    3 + List->IdxBuffer.BaseVertex,
                    3 + List->IdxBuffer.BaseVertex,
                    2 + List->IdxBuffer.BaseVertex,
                    0 + List->IdxBuffer.BaseVertex,
                };

                memcpy(Write, Indices, sizeof(Indices));

                List->IdxBuffer.WriteOffset += sizeof(Indices);
                List->IdxBuffer.BaseVertex  += 4;
            }

        } break;

        }
    }

    glNamedBufferSubData(List->VtxBuffer.Buffer, 0, 
                         List->VtxBuffer.FrameSize,
                         List->VtxBuffer.Cpu);

    glNamedBufferSubData(List->IdxBuffer.Buffer, 0,
                         List->IdxBuffer.FrameSize,
                         List->IdxBuffer.Cpu);

    // WARN: Obviously, cannot hardcode it like that, I need a simple scheme.
    jani_draw_command Command = {};
    Command.Count             = 30;

    List->Commands.Push(Command);
}

static inline void
BindResourceQueue(jani_backend_resource_queue *Queue)
{
    for(u32 Index = 0; Index < Queue->At; Index++)
    {
        backend_resource *Resource = Queue->Resources + Index;

        switch(Resource->Type)
        {

        case JANI_BACKEND_RESOURCE_TEXTURE:
        {
            glBindTextureUnit(Resource->BindSlot, Resource->Data.Texture.Id);
        } break;

        case JANI_BACKEND_RESOURCE_CBUFFER:
        {
            glBindBufferBase(GL_UNIFORM_BUFFER, Resource->BindSlot,
                             Resource->Data.CBuffer.Buffer);
        } break;

        default:
            break;
        }
    }
}

void
DrawPipelineCommands(jani_pipeline_state *State)
{
    glBindProgramPipeline(State->Pipeline);
    glBindVertexArray(State->VertexArrayObject);
    BindResourceQueue(&State->ResourceQueue);

    for(u32 Index = 0; Index < State->DrawList.Commands.Size; Index++)
    {
        jani_draw_command Command = State->DrawList.Commands[Index];

        glDrawElements(GL_TRIANGLES, Command.Count, GL_UNSIGNED_INT, (const void*)0);
    }

    State->DrawList.VtxBuffer.FrameSize   = 0;
    State->DrawList.VtxBuffer.WriteOffset = 0;

    State->DrawList.IdxBuffer.FrameSize   = 0;
    State->DrawList.IdxBuffer.BaseVertex  = 0;
    State->DrawList.IdxBuffer.WriteOffset = 0;

    State->DrawList.DrawInfos.Reset();
    State->DrawList.Commands.Reset();

}

static inline GLenum
GetShaderType(JANI_SHADER_TYPE Type)
{
    switch(Type)
    {

    case JANI_VERTEX_SHADER_BIT: return GL_VERTEX_SHADER;
    case JANI_PIXEL_SHADER_BIT : return GL_FRAGMENT_SHADER;

    default:
         Jani_Assert(!"Unknown shader type");
         return GL_NONE;

    }
}

static inline GLenum
GetShaderBit(JANI_SHADER_TYPE Type)
{
    switch(Type)
    {

    case JANI_VERTEX_SHADER_BIT: return GL_VERTEX_SHADER_BIT;
    case JANI_PIXEL_SHADER_BIT : return GL_FRAGMENT_SHADER_BIT;

    default:
         Jani_Assert(!"Unknown shader type");
         return GL_NONE;
    }
}

static void inline
CreateAndBindShaders(jani_pipeline_state *State, jani_shader_info *Shaders,
                     u32 ShaderCount)
{
    for(u32 Index = 0; Index < ShaderCount; Index++)
    {
        jani_shader_info Shader = Shaders[Index];

        if(Shader.Type & State->EnabledShaders)
        {
            return;
        }

        const GLchar** ByteCode   = (const GLchar**)&Shader.ByteCode;
        GLenum         NativeType = GetShaderType(Shader.Type);
        GLuint         Handle     = glCreateShaderProgramv(NativeType, 1, ByteCode);

        GLint Status;
        glGetProgramiv(Handle, GL_LINK_STATUS, &Status);
        if(!Status)
        {
            return;
        }

        GLenum ShaderBit = GetShaderBit(Shader.Type);
        glUseProgramStages(State->Pipeline, ShaderBit, Handle);

        State->EnabledShaders |= Shader.Type;
    }
}

static inline GLenum
GetNativeDataType(JANI_TYPE Type)
{
    switch(Type)
    {

    case JANI_U32: return GL_UNSIGNED_INT;
    case JANI_F32: return GL_FLOAT;

    default      : return GL_UNSIGNED_INT;

    };
}

static inline size_t
GetSizeOfNativeDataType(GLenum Type)
{
    switch(Type)
    {

        case GL_UNSIGNED_INT: return sizeof(u32);
        case GL_FLOAT       : return sizeof(f32);

        default             : return sizeof(u8);
    }
}

static inline void
CreateAndSetInputLayout(jani_pipeline_state *State, jani_shader_input *Inputs,
                        u32 InputCount)
{
    for(u32 Index = 0; Index < InputCount; Index++)
    {
        jani_shader_input Input = Inputs[Index];

        GLenum Type = GetNativeDataType(Input.Type);
        size_t Size = GetSizeOfNativeDataType(Type);

        glVertexArrayAttribFormat(State->VertexArrayObject, Index, Input.Count,
                                  Type, GL_FALSE, (GLuint)State->InputStride);
        glVertexArrayAttribBinding(State->VertexArrayObject, Index, Input.BufferIndex);
        glEnableVertexArrayAttrib (State->VertexArrayObject, Index);

        State->InputStride += (u32)(Input.Count * Size);

        switch(Input.GeneratorType)
        {

        case JANI_VERTEX_GEN_QUAD_POSITION_2D:
        {
            State->Generators[Index] = GenerateQuadVertex;
        } break;

        case JANI_VERTEX_GEN_QUAD_COLOR:
        {
            State->Generators[Index] = GenerateQuadColor;
        } break;

        default:
            Jani_Assert(!"Vertex generator type not implemented by the opengl backend");
            break;
        }
    }
}

static inline GLenum
GetBufferUsage(JANI_BUFFER_UPDATE_TYPE Type)
{
    switch(Type)
    {

    case JANI_BUFFER_UPDATE_PER_FRAME: return GL_DYNAMIC_DRAW;

    default:
    {
        Jani_Assert(!"Unknown buffer update type\n");
        return GL_DYNAMIC_DRAW;
    } break;

    }
}

static inline GLbitfield
GetBufferStorageFlags(JANI_BUFFER_UPDATE_TYPE Type)
{
    switch (Type)
    {

    case JANI_BUFFER_UPDATE_PER_FRAME: return GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT;

    default:
    {
        Jani_Assert(!"Unknown buffer update type\n");
        return GL_DYNAMIC_DRAW;
    } break;

    }
}

static inline void
CreateAndBindBuffers(jani_pipeline_state *State, jani_pipeline_buffer *Buffers,
                      u32 BufferCount)
{
    for(u32 Index = 0; Index < BufferCount; Index++)
    {
        jani_pipeline_buffer Buffer = Buffers[Index];

        switch(Buffer.BufferType)
        {

        case JANI_BUFFER_VERTEX:
        {
            glCreateBuffers(1, &State->DrawList.VtxBuffer.Buffer);

            GLenum Usage = GetBufferUsage(Buffer.UpdateType);
            glNamedBufferData(State->DrawList.VtxBuffer.Buffer,
                              Buffer.Size, nullptr, Usage);

            glVertexArrayVertexBuffer(State->VertexArrayObject, 0,
                                      State->DrawList.VtxBuffer.Buffer, 0,
                                      State->InputStride);

            State->DrawList.VtxBuffer.Size = Buffer.Size;
        } break;

        case JANI_BUFFER_INDEX:
        {
            glCreateBuffers(1, &State->DrawList.IdxBuffer.Buffer);

            GLenum Usage = GetBufferUsage(Buffer.UpdateType);
            glNamedBufferData(State->DrawList.IdxBuffer.Buffer, Buffer.Size,
                              nullptr, Usage);

            glVertexArrayElementBuffer(State->VertexArrayObject, 
                                       State->DrawList.IdxBuffer.Buffer);

            State->DrawList.IdxBuffer.Size = Buffer.Size;
        } break;

        }
    }
}

static inline backend_resource*
GetNextResource(jani_backend_resource_queue *Queue)
{
    Jani_Assert(Queue);

    if(Queue->At == Queue->Capacity)
    {
        Jani_Assert(!"Queue is already full");
        return nullptr;
    }

    backend_resource *Resource = Queue->Resources + Queue->At;
    ++Queue->At;

    return Resource;
}

static inline void
CreateAndBindResources(jani_pipeline_state *State, jani_resource_binding *Bindings,
                       u32 BindingCount)
{
    for(u32 Index = 0; Index < BindingCount; Index++)
    {
        jani_resource_binding Binding  = Bindings[Index];
        backend_resource     *Resource = GetNextResource(&State->ResourceQueue);

        if(!Resource) break;

        Resource->BindSlot = Binding.BindSlot;
        Resource->Type     = Binding.Type;

        u32 IdLength = StringLength(Binding.Id);
        if(IdLength < JANI_RESOURCE_ID_SIZE)
        {
            memcpy(Resource->Id, Binding.Id, IdLength);
        }
        else
        {
            Jani_Assert(!"Resource id exceeds maximum length.\n");
        }

        switch(Binding.Type)
        {

        case JANI_BACKEND_RESOURCE_TEXTURE:
        {
            Jani_Assert(Binding.Size == sizeof(jani_texture_info) &&
                        Binding.InitData);

            jani_texture_info *Info = (jani_texture_info*)Binding.InitData;

            if(Info->Data)
            {
            }
            else if(Info->Path)
            {
                i32 Width, Height, Channels;
                u8* Pixels = stbi_load((const char*)Info->Path, &Width, &Height,
                                       &Channels, 0);
                GLenum Format = (Channels == 4 ? GL_RGBA :
                                 Channels == 3 ? GL_RGB  :
                                 Channels == 1 ? GL_RED  : 
                                                 GL_RGB);
                GLenum InternalFormat = (Channels == 4 ? GL_RGBA8 :
                                         Channels == 3 ? GL_RGB8  :
                                                         GL_R8);

                backend_texture *Texture = &Resource->Data.Texture;
                glCreateTextures(GL_TEXTURE_2D, 1, &Texture->Id);
                glTextureStorage2D(Texture->Id, 1, InternalFormat, Width, Height);

                glTextureParameteri(Texture->Id, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTextureParameteri(Texture->Id, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTextureParameteri(Texture->Id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTextureParameteri(Texture->Id, GL_TEXTURE_MIN_FILTER,
                                    GL_LINEAR_MIPMAP_LINEAR);

                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTextureSubImage2D(Texture->Id, 0, 0, 0,
                                    Width, Height, Format, GL_UNSIGNED_BYTE,
                                    Pixels);

                stbi_image_free(Pixels);
            }
            else
            {
                Jani_Assert(!"Cannot create texture\n");
            }

        } break;

        // BUG: Flags in glMapNamedBufferRange should depend on the buffer usage.
        case JANI_BACKEND_RESOURCE_CBUFFER:
        {
            Jani_Assert(Binding.Size > 0);

            backend_constant_buffer *CBuffer = &Resource->Data.CBuffer;

            glCreateBuffers(1, &CBuffer->Buffer);

            GLbitfield Flags = GetBufferStorageFlags(Binding.Extra.UpdateType);
            glNamedBufferStorage(CBuffer->Buffer, Binding.Size, Binding.InitData,
                                 Flags);

            CBuffer->SizeOfData = Binding.Size;
        } break;

        default:
        {
            Jani_Assert(!"Invalid resource type\n");
        } break;

        }
    }
}

// WARN: This thing is a mess. Immediate mode API should fix this easily
// idea: SetPipeline(pipeline_handle *Handle) if handle == 0, create one?

jani_pipeline_handle
CreatePipeline(jani_context *Context, jani_pipeline_info Info)
{
    Jani_Assert(Context);

    jani_backend *Backend = Context->Backend;

    if(!Backend->Initialized)
    {
        jani_allocator *A = &Context->Allocator;

        Backend->PipelineBufferSize = 4;
        Backend->NextPipelineID     = 1;
        Backend->StateIDs           = (u16*)A->Allocate(4 * sizeof(u16));

        // WARN: This overwrites the default pipeline slot. So be careful.
        memset(Backend->StateIDs, INVALID_ID, 4 * sizeof(u16)); 

        Backend->States = (jani_pipeline_state*)
            A->Allocate(4 * sizeof(jani_pipeline_state));

        Backend->Initialized = true;
    }

    jani_pipeline_state *State    = nullptr;
    u16                  Id       = Backend->NextPipelineID;
    u16                  StateIdx = Id % Backend->PipelineBufferSize;
    jani_pipeline_handle Handle   = MAKE_PIPELINE_HANDLE(INVALID_ID, 0);

    u32 Iterations = 0;
    while(Iterations < Backend->PipelineBufferSize)
    {
        if(Backend->StateIDs[StateIdx] == INVALID_ID)
        {
            Handle = MAKE_PIPELINE_HANDLE(Id, StateIdx);
            State  = Backend->States + StateIdx;

            Backend->StateIDs[StateIdx] = Id;
            break;
        }

        StateIdx = (StateIdx + 1) & Backend->PipelineBufferSize;
        Iterations++;
    }

    if(State)
    {
        glCreateVertexArrays(1, &State->VertexArrayObject);
        glCreateProgramPipelines(1, &State->Pipeline);

        jani_backend_resource_queue *Queue = &State->ResourceQueue;
        jani_backend_draw_list      *List  = &State->DrawList;
        jani_allocator              *A     = &Context->Allocator;

        Queue->Capacity  = Info.BindingCount;
        Queue->Resources = (backend_resource*)
            A->Allocate(Queue->Capacity * sizeof(backend_resource));

        List->DrawInfos = JaniBumper<jani_draw_info>   (10*sizeof(jani_draw_info));
        List->Commands  = JaniBumper<jani_draw_command>(10*sizeof(jani_draw_command));


        State->InputCount     = Info.InputCount;
        State->Generators     = (jani_vertex_generator*)
            A->Allocate(State->InputCount*sizeof(jani_vertex_generator));

        CreateAndBindShaders   (State, Info.Shaders , Info.ShaderCount );
        CreateAndSetInputLayout(State, Info.Inputs  , Info.InputCount  );
        CreateAndBindBuffers   (State, Info.Buffers , Info.BufferCount );
        CreateAndBindResources (State, Info.Bindings, Info.BindingCount);

        List->VtxBuffer.Cpu = (u8*)A->Allocate(List->VtxBuffer.Size);
        List->IdxBuffer.Cpu = (u8*)A->Allocate(List->IdxBuffer.Size);

        return Handle;
    }

    return 0; // Corresponds to the default pipeline
}

void inline
SetPipelineState(jani_backend *Backend, jani_pipeline_handle PipelineHandle)
{
    Backend->ActivePipeline = PipelineHandle;
}

// ==================================
// RESOURCES
// ==================================

void
UpdatePipelineResource(jani_context *Context, const char *Id, void *Data,
                       size_t Size, jani_pipeline_handle Handle)
{
    u16                   PIndex = GET_INDEX_FROM_HANDLE(Handle);
    jani_pipeline_state  *State  = Context->Backend->States + PIndex;

    // WARN
    for(u32 Index = 0; Index < State->ResourceQueue.Capacity; Index++)
    {
        backend_resource *Resource = State->ResourceQueue.Resources + Index;

        if(Resource->Type == JANI_BACKEND_RESOURCE_NONE) continue;

        if(strcmp(Resource->Id, Id) == 0)
        {
            switch(Resource->Type)
            {

            case JANI_BACKEND_RESOURCE_CBUFFER:
            {
                GLuint Buffer     = Resource->Data.CBuffer.Buffer;
                size_t BufferSize = Resource->Data.CBuffer.SizeOfData;

                Jani_Assert(Size <= BufferSize);

                    void *WritePointer = glMapNamedBuffer(Buffer, GL_WRITE_ONLY);
                    if(WritePointer)
                    {
                        memcpy(WritePointer, Data, Size);
                        glUnmapNamedBuffer(Buffer);
                    }
                    else
                    {
                        Jani_Assert(!"Could not update the resource.\n");
                    }
            } break;

            case JANI_BACKEND_RESOURCE_TEXTURE:
            {
                Jani_Assert(!"Cannot update texture.\n");
            } break;

            }
        }
    }
}

// ==================================
// IMMEDIATE MODE API TESTING
// ==================================

}
