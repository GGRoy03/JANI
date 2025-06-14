#pragma once

#include <GL/gl.h>
#include "glcorearb.h"
#include "wgltext.h"

#include "jani.h"

#pragma comment (lib, "opengl32.lib")

namespace JANI
{

struct backend_texture
{
    GLuint Id;
};

struct backend_constant_buffer
{
    size_t SizeOfData;
    GLuint Buffer;
};

struct backend_resource
{
    JANI_BACKEND_RESOURCE_TYPE Type;
    u32                        BindSlot;
    char                       Id[JANI_RESOURCE_ID_SIZE];

    union
    {
        backend_texture         Texture;
        backend_constant_buffer CBuffer;
    } Data;
};

struct jani_backend_resource_queue
{
    backend_resource *Resources;
    u32               Capacity;
    u32               At;
};

struct jani_backend
{
    u32                  PipelineBufferSize;
    u16                 *StateIDs;
    jani_pipeline_state *States;
    u16                  NextPipelineID;
    jani_pipeline_handle ActivePipeline;

    bool Initialized;
};

struct jani_backend_vertex_buffer
{
    GLuint Buffer;

    u8*    Cpu;
    size_t WriteOffset;

    size_t FrameSize;
    size_t Size;
};

struct jani_backend_index_buffer
{
    GLuint Buffer;

    u8*    Cpu;
    size_t WriteOffset;

    u32 BaseVertex;

    size_t FrameSize;
    size_t Size;
};

struct jani_backend_draw_list
{
    jani_backend_vertex_buffer VtxBuffer;
    jani_backend_index_buffer  IdxBuffer;

    JaniBumper<jani_draw_info>    DrawInfos;
    JaniBumper<jani_draw_command> Commands;
};

struct jani_pipeline_state
{
    GLuint VertexArrayObject;
    GLuint Pipeline;
    u32    InputStride;
    u32    InputCount;

    jani_backend_draw_list      DrawList;
    jani_backend_resource_queue ResourceQueue;

    jani_vertex_generator *Generators;

    jani_bit_field EnabledShaders;
};

jani_pipeline_handle
CreatePipeline(jani_context *Context, jani_pipeline_info PipelineInfo);

void inline
SetPipelineState(jani_backend *Backend, jani_pipeline_handle PipelineHandle);

}
