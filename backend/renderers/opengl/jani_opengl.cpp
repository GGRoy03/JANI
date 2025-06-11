#include "jani_opengl.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

namespace JANI
{

sorted_metas
SortDraws(jani_context *Context)
{
    sorted_metas Sorted = {};

    // For now we allocate a new array of commands. These are sorted.
    u32   CommandCount   = Context->CommandMetas.Size;
    auto *SortedMetas = (jani_draw_meta*)
        Context->Allocator.Allocate(CommandCount * sizeof(jani_draw_meta));

    for(u32 Index = 0; Index < CommandCount; Index++)
    {
        SortedMetas[Index] = Context->CommandMetas.Values[Index];
    }

    // Simply use insertion sort for now.
    i32 Left  = 0;
    i32 Right = CommandCount - 1;
    for(i32 Index = Left; Index < Right; Index++)
    {
        jani_draw_meta *Meta = SortedMetas + Index;

        auto Key      = Meta->PipelineHandle; 
        i32  SubIndex = Index - 1;

        while(SubIndex >= Left && Meta->PipelineHandle > Key)
        {
            SortedMetas[SubIndex + 1] = SortedMetas[SubIndex];
            SubIndex = SubIndex - 1;
        }
        SortedMetas[SubIndex + 1] = *Meta; 
    }

    Sorted.MetaCount = CommandCount;
    Sorted.Metas     = SortedMetas;

    return Sorted;
}

// NOTE: We could also consider doing 1 list / type of commands and just
// loop over those perhaps?
JaniBumper<jani_draw_command>
BuildAndCopyData(jani_context *Context, sorted_metas *Sorted)
{
    jani_backend *Backend = Context->Backend;

    size_t     AllocSize = Sorted->MetaCount * sizeof(jani_draw_command);
    JaniBumper Commands  = JaniBumper<jani_draw_command>(AllocSize); // BUG: Leak?

    jani_pipeline_state *CurrentPipeline = nullptr;

    for(u32 Index = 0; Index < Sorted->MetaCount; Index++)
    {
        jani_draw_meta *Meta = Sorted->Metas + Index;

        u16   PipelineIndex = GET_INDEX_FROM_HANDLE(Meta->PipelineHandle);
        auto *PState        = Backend->States + PipelineIndex;

        if(PState != CurrentPipeline)
        {
            if(PState->FrameVertexSize > PState->VertexBufferSize)
            {
                PState->VertexBufferSize = PState->FrameVertexSize + Kilobytes(5);
                glNamedBufferData(PState->VertexBuffer,
                                  PState->VertexBufferSize,
                                  nullptr,
                                  GL_DYNAMIC_DRAW);
            }

            if(PState->FrameIndexSize > PState->IndexBufferSize)
            {
                PState->IndexBufferSize = PState->FrameIndexSize + Kilobytes(5);
                glNamedBufferData(PState->IndexBuffer,
                                  PState->IndexBufferSize,
                                  nullptr,
                                  GL_DYNAMIC_DRAW);
            }

            CurrentPipeline = PState;
        }

        switch(Meta->Type)
        {
        case JANI_DRAW_META_TEXT:
        {
        } break;

        case JANI_DRAW_META_QUAD:
        {
            u32 VertexCount = 4;
            u32 IndexCount  = 6;

            f32 QuadVerts[8] =
            {
                -0.25f, 0.25f, // Top-left
                 0.25f, 0.25f, // Top-right
                -0.25f,-0.25f, // Bottom right
                 0.25f,-0.25f, // Bottom left
            };

            glNamedBufferSubData(PState->VertexBuffer, PState->FrameDataOffset,
                                 sizeof(QuadVerts), QuadVerts);

            GLuint QuadIndices[6] = {
                0, 1, 3,
                3, 2, 0
            };

            glNamedBufferSubData(
                PState->IndexBuffer,
                PState->FrameIndexOffset,
                sizeof(QuadIndices),
                QuadIndices
            );

            jani_draw_command Command = {};
            Command.Type              = JANI_DRAW_COMMAND_INDEXED_OFFSET;
            Command.Count             = IndexCount;
            Command.Offset            = PState->FrameIndexOffset;
            Command.BaseVertex        = PState->FrameBaseVertex;
            Command.PipelineState     = PState;

            Commands.Push(Command);

            PState->FrameIndexOffset += IndexCount * sizeof(u32);
            PState->FrameBaseVertex  += VertexCount;
            PState->FrameDataOffset  += sizeof(QuadVerts);
        } break;

        default:
        {
        } break;

        }
    }

    Context->Allocator.Free(Sorted->Metas, Sorted->MetaCount * sizeof(jani_draw_meta));

    return Commands;
}

static inline void
BindResourceQueue(backend_resouce_queue *Queue)
{
    for(u32 Index = 0; Index < Queue->Count; Index++)
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
                             Resource->Data.CBuffer.Id);
        } break;

        }
    }
}

void 
ExecuteDrawCommands(JaniBumper<jani_draw_command> *Commands)
{
    jani_pipeline_state *CurrentPipelineState = nullptr;

    for(u32 Index = 0; Index < Commands->Size; Index++)
    {
        jani_draw_command *Command = Commands->Values + Index;

        if(Command->PipelineState != CurrentPipelineState)
        {
            // NOTE: Could also bind the first pipeline before entering the loop
            // to avoid the if check?
            if(CurrentPipelineState)
            {
                CurrentPipelineState->FrameVertexSize  = 0;
                CurrentPipelineState->FrameIndexSize   = 0;
                CurrentPipelineState->FrameIndexOffset = 0;
                CurrentPipelineState->FrameDataOffset  = 0;
                CurrentPipelineState->FrameBaseVertex  = 0;
            }

            glBindProgramPipeline(Command->PipelineState->Pipeline);
            glBindVertexArray(Command->PipelineState->VertexArrayObject);

            BindResourceQueue(&Command->PipelineState->ResourceQueue);

            CurrentPipelineState = Command->PipelineState;

        }

        switch(Command->Type)
        {

        case JANI_DRAW_COMMAND_INDEXED_OFFSET:
        {
            glDrawElementsBaseVertex(GL_TRIANGLES, Command->Count, GL_UNSIGNED_INT,
                                    (const void*)Command->Offset, Command->BaseVertex);
        } break;

        }
    }

    CurrentPipelineState->FrameVertexSize = 0;
    CurrentPipelineState->FrameIndexSize = 0;
    CurrentPipelineState->FrameIndexOffset = 0;
    CurrentPipelineState->FrameDataOffset = 0;
    CurrentPipelineState->FrameBaseVertex = 0;
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

//
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

static void inline
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
            glCreateBuffers(1, &State->VertexBuffer);

            GLenum Usage = GetBufferUsage(Buffer.UpdateType);
            glNamedBufferData(State->VertexBuffer, Buffer.Size, nullptr, Usage);

            glVertexArrayVertexBuffer(State->VertexArrayObject, 0,
                                      State->VertexBuffer, 0, State->InputStride);

            State->VertexBufferSize = Buffer.Size;
        } break;

        case JANI_BUFFER_INDEX:
        {
            glCreateBuffers(1, &State->IndexBuffer);

            GLenum Usage = GetBufferUsage(Buffer.UpdateType);
            glNamedBufferData(State->IndexBuffer, Buffer.Size, nullptr, Usage);

            glVertexArrayElementBuffer(State->VertexArrayObject, State->IndexBuffer);

            State->IndexBufferSize = Buffer.Size;
        } break;

        }
    }
}

static inline backend_resource*
GetNextResource(backend_resource_queue *Queue)
{
    Jani_Assert(Queue);

    if(Queue->At == Queue->Capacity)
    {
        Jani_Assert(!"Queue is already full");
        return nullptr;
    }

    backend_resource *Resource = Queue->Resources + Queue->At;
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

                backend_texture *Texture = &Resource->Data.Texture;
                glCreateTextures(GL_TEXTURE_2D, 1, &Texture->Id);

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

        // BUG: Flags in glMapNamedBufferRange should depend on the buffer
        // usage.
        case JANI_BACKEND_RESOURCE_CBUFFER:
        {
            Jani_Assert(Binding.Size > 0);

            backend_constant_buffer *CBuffer = &Resource->Data.CBuffer;

            glCreateBuffers(1, &CBuffer->Buffer);

            GLenum Usage = GetBufferUsage(Binding.Extra.UpdateType);
            glNamedBufferStorage(CBuffer->Buffer, Binding.Size, Binding.InitData,
                                 Usage);

            CBuffer->SizeOfData = Binding.Size;
            CBuffer->DataPointer = 
                        glMapNamedBufferRange(CBuffer->Buffer, 0, Binding.Size,
                                              GL_DYNAMIC_STORAGE_BIT|GL_MAP_WRITE_BIT);
        } break;

        default:
        {
            Jani_Assert(!"Invalid resource type\n");
        } break;

        }
    }
}

jani_pipeline_handle
CreatePipeline(jani_context *Context, jani_pipeline_info Info)
{
    Jani_Assert(Context);

    jani_backend   *Backend   = Context->Backend;

    // NOTE: We used to initialize the backend here, but it is a mistake
    // so do it elsewhere.

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

        backend_resource_queue *Queue = &State->ResourceQueue;
        jani_allocator         *A     = &Context->Allocator;
        Queue->Capacity  = Info.BindingCount;
        Queue->Resources = (backend_resource*)
            A->Allocate(Queue->Capacity * sizeof(backend_resource));

        CreateAndBindShaders   (State, Info.Shaders , Info.ShaderCount );
        CreateAndSetInputLayout(State, Info.Inputs  , Info.InputCount  );
        CreateAndBindBuffers   (State, Info.Buffers , Info.BufferCount );
        CreateAndBindResources (State, Info.Bindings, Info.BindingCount);

        return Handle;
    }

    return 0; // Corresponds to the default pipeline
}

void inline
SetPipelineState(jani_backend *Backend, jani_pipeline_handle PipelineHandle)
{
    Backend->ActivePipeline = PipelineHandle;
}

}
