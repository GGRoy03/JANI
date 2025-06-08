#pragma once

#include "jani_internals.h"

namespace JANI
{

// ===========================================
// Pipeline 
// ===========================================

struct jani_pipeline_state;
struct jani_context;

using  jani_pipeline_handle = u32;
using  jani_bit_field       = u32;

#define INVALID_ID 0xFFFF
#define MAKE_PIPELINE_HANDLE(ID, INDEX) (jani_pipeline_handle)((ID << 16) | INDEX)
#define GET_ID_FROM_HANDLE(Handle)      (Handle >> 16)
#define GET_INDEX_FROM_HANDLE(Handle)   (u16)(Handle & 0xFFFF0000)

#ifndef JANI_PIPELINE_BUFFER_DEFAULT_SIZE 
#define JANI_PIPELINE_BUFFER_DEFAULT_SIZE 4
#endif

struct jani_backend
{
    u32                  PipelineBufferSize;
    u16                 *StateIDs;
    jani_pipeline_state *States;
    u16                  NextPipelineID;
    jani_pipeline_handle ActivePipeline;

    bool Valid;
};

enum JANI_SHADER_TYPE : u32
{
    JANI_VERTEX_SHADER_BIT = 1 << 0,
    JANI_PIXEL_SHADER_BIT  = 1 << 1,
};

enum JANI_TYPE : u32
{
    JANI_U32,
    JANI_F32,
};

struct jani_shader_input
{
    JANI_TYPE Type;
    u32       BindSlot;
    u32       Count;
};

struct jani_pipeline_info
{
    const char *VertexShaderByteCode;
    const char *PixelShaderByteCode;

    size_t DefaultVertexBufferSize;
    size_t DefaultIndexBufferSize;

    jani_shader_input* Inputs;
    u32                InputCount;
};

// ===========================================
// Drawing 
// ===========================================

enum JANI_DRAW_META_TYPE
{ 
    JANI_DRAW_META_NONE,

    JANI_DRAW_META_MESH,
    JANI_DRAW_META_INSTANCED_MESH,
    JANI_DRAW_META_TEXT,
    JANI_DRAW_META_QUAD,
};

struct draw_text_payload
{
    u8  *Text;
    f32 PositionX;
    f32 PositionY;
};

struct draw_quad_payload
{
    u32 SizeX;
    u32 SizeY;
    u32 TopLeftX;
    u32 TopLeftY;
};

struct jani_draw_meta
{
    JANI_DRAW_META_TYPE  Type;
    jani_pipeline_handle PipelineHandle;

    union
    {
        draw_text_payload Text;
        draw_quad_payload Quad;
    } Payload;
};

enum JANI_DRAW_COMMAND_TYPE
{
    JANI_DRAW_COMMAND_NONE,
    JANI_DRAW_COMMAND_INDEXED_OFFSET,
};

struct jani_draw_command
{
    JANI_DRAW_COMMAND_TYPE Type;
    u32                    Count;
    u64                    Offset;
    u32                    BaseVertex;
    jani_pipeline_state   *PipelineState;

};


// ===========================================
// Frame
// ===========================================

struct jani_context
{
    jani_allocator             Allocator;
    jani_backend*              Backend;
    JaniBumper<jani_draw_meta> CommandMetas;
    i32                        SizeX;
    i32                        SizeY;

    bool Initialized;
};

struct sorted_metas
{
    jani_draw_meta *Metas;
    u32             MetaCount;
};


void BeginUIFrame(jani_context *Context);
void DrawBox     (jani_context *Context);
void EndUIFrame  (jani_context *Context);

}
