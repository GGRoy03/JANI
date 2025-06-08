#pragma once

#include <GL/gl.h>
#include "glcorearb.h"
#include "wgltext.h"

#include "jani.h"

#pragma comment (lib, "opengl32.lib")

namespace JANI
{

struct jani_pipeline_state
{
    GLuint Pipeline;
    u32    InputStride;

    GLuint IndexBuffer;
    GLuint VertexBuffer;
    GLuint VertexArrayObject;

    size_t VertexBufferSize;
    size_t IndexBufferSize;

    // NOTE: These need to be reset every frame. Question is: where?
    size_t  FrameVertexSize;
    size_t  FrameIndexSize;
    GLuint  FrameIndexOffset;
    GLsizei FrameDataOffset;
    GLint   FrameBaseVertex;

    jani_bit_field EnabledShaders;
};

jani_pipeline_handle
CreatePipeline(jani_context *Context, jani_pipeline_info PipelineInfo);

void inline
SetPipelineState(jani_backend *Backend, jani_pipeline_handle PipelineHandle);

void 
ExecuteDrawCommands(JaniBumper<jani_draw_command> *Commands);

}
