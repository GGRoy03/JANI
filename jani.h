#pragma once

#include "jani_internals.h"

#ifndef JANI_DEFAULT_RESOURCE_COUNT
#define JANI_DEFAULT_RESOURCE_COUNT 2
#endif

namespace JANI
{

struct jani_backend;
struct jani_pipeline_state;
struct jani_context;

// ===========================================
// Platform 
// ===========================================

struct jani_platform_read_result
{
    u8*    Content;
    size_t ContentSize;
};

using jani_platform_read_file = 
jani_platform_read_result(*)(const char* Path, jani_allocator *Allocator);

void *JANI_GLOBAL_WINDOW_HANDLE = nullptr;

struct jani_platform
{
    jani_platform_read_file JaniReadFile;

    void *WindowHandle;
    bool Initialized;
};

// ===========================================
// Pipeline 
// ===========================================

using  jani_pipeline_handle = u32;
using  jani_bit_field       = u32;

#define INVALID_ID 0xFFFF
#define MAKE_PIPELINE_HANDLE(ID, INDEX) (jani_pipeline_handle)((ID << 16) | INDEX)
#define GET_ID_FROM_HANDLE(Handle)      (Handle >> 16)
#define GET_INDEX_FROM_HANDLE(Handle)   (u16)(Handle & 0xFFFF0000)

#ifndef JANI_DEFAULT_PIPELINE_COUNT
#define JANI_DEFAULT_PIPELINE_COUNT 4
#endif

#ifndef JANI_DEFAULT_GLOBAL_RESOURCE_COUNT
#define JANI_DEFAULT_GLOBAL_RESOURCE_COUNT 1
#endif

#ifndef JANI_DEFAULT_STICKY_RESOURCE_COUNT
#define JANI_DEFAULT_STICKY_RESOURCE_COUNT 1
#endif

#define JANI_NO_FLAGS 0

enum JANI_BACKEND_RESOURCE_TYPE
{
    JANI_BACKEND_RESOURCE_NONE,
    JANI_BACKEND_RESOURCE_TEXTURE,
    JANI_BACKEND_RESOURCE_CBUFFER,
};

enum JANI_BUFFER_TYPE
{
    JANI_BUFFER_NONE,
    JANI_BUFFER_VERTEX,
    JANI_BUFFER_INDEX,
};

enum JANI_BUFFER_UPDATE_TYPE
{
    JANI_BUFFER_UPDATE_NONE,
    JANI_BUFFER_UPDATE_PER_FRAME,
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
    u32       Count;
    u32       BufferIndex;
};

struct jani_shader_info
{
    const char      *ByteCode;
    JANI_SHADER_TYPE Type;
};

struct jani_texture_info
{
    const char* Path;
    char*       Data;
};

struct jani_pipeline_buffer
{
    size_t                  Size;
    JANI_BUFFER_TYPE        BufferType;
    JANI_BUFFER_UPDATE_TYPE UpdateType;
};

struct jani_resource_binding
{
    JANI_BACKEND_RESOURCE_TYPE Type;
    u32                        BindSlot;
    size_t                     Size;
    void                      *InitData;

    union
    {
        JANI_BUFFER_UPDATE_TYPE UpdateType;
    } Extra;
};

struct jani_pipeline_info
{
    jani_shader_info  *Shaders;
    u32               ShaderCount;

    jani_shader_input *Inputs;
    u32               InputCount;

    jani_pipeline_buffer *Buffers;
    u32                   BufferCount;

    jani_resource_binding *Bindings;
    u32                    BindingCount;
};

// TODO: Reserve the pipeline_handle 0 as the default one.

// ===========================================
// Drawing 
// ===========================================

enum JANI_DRAW_TYPE
{ 
    JANI_DRAW_NONE,

    JANI_DRAW_MESH,
    JANI_DRAW_INSTANCED_MESH,
    JANI_DRAW_TEXT,
    JANI_DRAW_QUAD,
};

struct draw_text_payload
{
    char *Text;
    u32  Length;
};

struct draw_quad_payload
{
    u32 SizeX;
    u32 SizeY;
    u32 TopLeftX;
    u32 TopLeftY;
};

struct jani_draw_info
{
    JANI_DRAW_TYPE DrawType;
    u32            VtxBufferTarget;
    u32            IdxBufferTarget;

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
    size_t                 Offset;
    u32                    BaseVertex;
};

// ===========================================
// Text 
// ===========================================

struct jani_glyph
{
    u32 ID;
    u32 PositionInAtlasX;
    u32 PositionInAtlasY;
    u32 SizeX;
    u32 SizeY;
    i32 OffsetX;
    i32 OffsetY;
    u32 XAdvance;
};

struct jani_font_map
{
    jani_glyph *Glyphs;
    u32         GlyphCount;
    u32         TextureSizeX;
    u32         TextureSizeY;
    u32         LineHeight;

    bool Loaded;
    bool Initialized;
};

jani_font_map
LoadFontMap(char* Path);

void
SetFontMap(jani_context *Context, jani_font_map *Map);

// ===========================================
// Camera 
// ===========================================

struct jani_camera_2D
{
    jani_mat4 Orthohraphic;
};

// ===========================================
// Frame
// ===========================================

struct jani_context
{
    jani_allocator Allocator;
    jani_backend*  Backend;

    // Still a bit messy.
    jani_font_map *ActiveFontMap;
    jani_platform  Platform;
    jani_camera_2D Camera;

    i32 ClientWidth;
    i32 ClientHeight;

    bool Initialized;
};

void BeginUIFrame(jani_context *Context);
void DrawBox     (jani_context *Context);
void EndUIFrame  (jani_context *Context);
void DrawText    (jani_context *Context, char* Text);

}
