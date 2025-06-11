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
    void  *DataPointer;
};

struct backend_resource
{
    JANI_BACKEND_RESOURCE_TYPE Type;
    u32                        BindSlot;

    union
    {
        backend_texture         Texture;
        backend_constant_buffer CBuffer;
    } Data;
};

struct backend_resource_queue
{
    backend_resource *Resources;
    u32               Capacity;
    u32               At;
};

struct jani_backend
{
    // Pipelines
    u32                  PipelineBufferSize;
    u16                 *StateIDs;
    jani_pipeline_state *States;
    u16                  NextPipelineID;
    jani_pipeline_handle ActivePipeline;

    backend_resource_queue GlobalResourceQueue;

    bool Valid;
};

struct jani_pipeline_state
{
    GLuint Pipeline;
    u32    InputStride;

    GLuint IndexBuffer;
    GLuint VertexBuffer;
    GLuint VertexArrayObject;

    size_t VertexBufferSize; // NOTE: Isn't that limited to 1/pipeline?
    size_t IndexBufferSize;

    size_t  FrameVertexSize;
    size_t  FrameIndexSize;
    GLuint  FrameIndexOffset;
    GLsizei FrameDataOffset;
    GLint   FrameBaseVertex;

    backend_resource_queue ResourceQueue;

    jani_bit_field EnabledShaders;
};

jani_pipeline_handle
CreatePipeline(jani_context *Context, jani_pipeline_info PipelineInfo);

void inline
SetPipelineState(jani_backend *Backend, jani_pipeline_handle PipelineHandle);

void 
ExecuteDrawCommands(JaniBumper<jani_draw_command> *Commands);

}
