#pragma once
#pragma message("jani.h included!")

#include "jani_internals.h"

#include <map>

namespace JANI
{

// ===========================================
// Pipeline 
// ===========================================

struct jani_context;
using  jani_pipeline_handle = u32;
using  jani_bit_field       = u32;

enum JANI_SHADER_TYPE : u32
{
    JANI_VERTEX_SHADER_BIT = 1 << 0,
    JANI_PIXEL_SHADER_BIT  = 1 << 1,
};

struct jani_pipeline_info
{
    u8 *VertexShaderByteCode;
    u8 *PixelShaderByteCode;
};

// ===========================================
// Drawing 
// ===========================================

using pipeline_handle = u64;

enum JANI_DRAW_COMMAND_TYPE
{ 
    JANI_DRAW_COMMAND_NONE,

    JANI_DRAW_COMMAND_MESH,
    JANI_DRAW_COMMAND_INSTANCED_MESH,
    JANI_DRAW_COMMAND_TEXT,
    JANI_DRAW_COMMAND_QUAD,
};

struct draw_text_payload
{
    u8  *Text;
    f32 PositionX;
    f32 PositionY;
};

struct jani_draw_command
{
    JANI_DRAW_COMMAND_TYPE Type;
    jani_pipeline_handle   Handle;

    union
    {
        draw_text_payload Text;
    } Payload;

};

void 
ExecuteDrawCommands(JaniBumper<jani_draw_command> *Commands);

// ===========================================
// Frame
// ===========================================

struct jani_backend;

struct jani_context
{
    jani_backend*                 Backend;
    JaniBumper<jani_draw_command> Commands;
    i32                           SizeX;
    i32                           SizeY;

    bool Initialized;
};

void BeginUIFrame(jani_context *Context);
void EndUIFrame  (jani_context *Context);

}
