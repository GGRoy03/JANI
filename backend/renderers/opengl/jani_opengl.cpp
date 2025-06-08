#include "jani_opengl.h"

#include <algorithm> // For sorting

namespace JANI
{

static GLenum
GetNativeType(JANI_TYPE JaniType)
{
    switch(JaniType)
    {

    case JANI_U32: return GL_UNSIGNED_INT;
    case JANI_F32: return GL_FLOAT;

    default      : return GL_UNSIGNED_INT;

    };
}

static size_t
GetSizeOfNativeType(GLenum NativeType)
{
    switch(NativeType)
    {
        case GL_UNSIGNED_INT: return sizeof(u32);
        case GL_FLOAT       : return sizeof(f32);
        default             : return sizeof(u8);
    }
}

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
    i32 Left              = 0;
    i32 Right             = CommandCount - 1;
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
BuildAndCopyData(jani_backend *Backend, sorted_metas *Sorted)
{
    size_t     AllocSize = Sorted->MetaCount * sizeof(jani_draw_command);
    JaniBumper Commands  = JaniBumper<jani_draw_command>(AllocSize);

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

    return Commands;
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

jani_pipeline_handle
CreatePipeline(jani_context *Context, jani_pipeline_info PipelineInfo)
{
    Jani_Assert(Context && PipelineInfo.Inputs);

    jani_backend *Backend = Context->Backend;

    if(!Backend->Valid)
    {
        Backend->PipelineBufferSize = JANI_PIPELINE_BUFFER_DEFAULT_SIZE;
        Backend->StateIDs           = (u16*) 
            Context->Allocator.Allocate(Backend->PipelineBufferSize * sizeof(u16));
        Backend->States             = (jani_pipeline_state*)
            Context->Allocator.Allocate(Backend->PipelineBufferSize *
                                        sizeof(jani_pipeline_state));
        Backend->NextPipelineID     = 0;
        Backend->ActivePipeline     = INVALID_ID;

        for (u32 Index = 0; Index < Backend->PipelineBufferSize; Index++)
        {
            Backend->StateIDs[Index] = INVALID_ID;
        }

        Backend->Valid              = true;
    }

    jani_pipeline_state  *State     = nullptr;
    u16                  Id         = Backend->NextPipelineID;
    u16                  StateIndex = Id % Backend->PipelineBufferSize;
    jani_pipeline_handle Handle     = MAKE_PIPELINE_HANDLE(INVALID_ID, 0);

    u32 Iterations = 0;
    while(Iterations < Backend->PipelineBufferSize)
    {
        if(Backend->StateIDs[StateIndex] == INVALID_ID)
        {
            Handle = MAKE_PIPELINE_HANDLE(Id, StateIndex);
            State  = Backend->States + StateIndex;

            Backend->StateIDs[StateIndex] = Id;
            break;
        }

        StateIndex = (StateIndex + 1) & Backend->PipelineBufferSize;
        Iterations++;
    }

    if(State)
    {
        glCreateVertexArrays(1, &State->VertexArrayObject);
        glGenProgramPipelines(1, &State->Pipeline);

        if(PipelineInfo.VertexShaderByteCode)
        {
            GLuint ShaderHandle = 
                glCreateShaderProgramv(GL_VERTEX_SHADER, 1,
                                       (const GLchar**)&PipelineInfo.VertexShaderByteCode);

            // TODO: Log error on failure 
            GLint Status;
            glGetProgramiv(ShaderHandle, GL_LINK_STATUS, &Status);
            if(!Status)
            {
            }

            glUseProgramStages(State->Pipeline, GL_VERTEX_SHADER_BIT, ShaderHandle);

            State->EnabledShaders = JANI_VERTEX_SHADER_BIT;
        }

        if(PipelineInfo.PixelShaderByteCode)
        {
            GLuint ShaderHandle = 
                glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, 
                                       (const GLchar**)&PipelineInfo.PixelShaderByteCode);

            // TODO: Log error on failure 
            GLint Status;
            glGetProgramiv(ShaderHandle, GL_LINK_STATUS, &Status);
            if(!Status)
            {
            }

            glUseProgramStages(State->Pipeline, GL_FRAGMENT_SHADER_BIT, ShaderHandle);

            State->EnabledShaders |= JANI_PIXEL_SHADER_BIT;
        }

        GLuint InputOffset = 0;
        for(u32 Index = 0; Index < PipelineInfo.InputCount; Index++)
        {
            jani_shader_input *Input = PipelineInfo.Inputs + Index;

            GLenum Type       = GetNativeType(Input->Type);
            size_t SizeOfType = GetSizeOfNativeType(Type);

            glVertexArrayAttribFormat(State->VertexArrayObject, Index, Input->Count,
                                      Type, GL_FALSE, InputOffset); 
            glVertexArrayAttribBinding(State->VertexArrayObject, Index,
                                       Input->BindSlot);
            glEnableVertexArrayAttrib(State->VertexArrayObject, Index);

            InputOffset += (GLuint)(Input->Count * SizeOfType);
        }
        State->InputStride = InputOffset;

        if(PipelineInfo.DefaultVertexBufferSize > 0)
        {
            glCreateBuffers(1, &State->VertexBuffer);
            glNamedBufferData(State->VertexBuffer,
                              PipelineInfo.DefaultVertexBufferSize,
                              NULL,
                              GL_DYNAMIC_DRAW);
            glVertexArrayVertexBuffer(State->VertexArrayObject, 0, State->VertexBuffer,
                                      0, State->InputStride);
            State->VertexBufferSize = PipelineInfo.DefaultVertexBufferSize;
        }

        if(PipelineInfo.DefaultIndexBufferSize > 0)
        {
            glCreateBuffers(1, &State->IndexBuffer);
            glNamedBufferData(State->IndexBuffer,
                              PipelineInfo.DefaultIndexBufferSize,
                              NULL,
                              GL_DYNAMIC_DRAW);
            glVertexArrayElementBuffer(State->VertexArrayObject, State->IndexBuffer);
            State->IndexBufferSize = PipelineInfo.DefaultIndexBufferSize;
        }
    }
    else
    {
        // WARN: If we reach here it means that we don't have any pipeline slots
        // remaining / we have a bug. So log it.
    }

    return Handle;
}

void inline
SetPipelineState(jani_backend *Backend, jani_pipeline_handle PipelineHandle)
{
    Backend->ActivePipeline = PipelineHandle;
}

}
