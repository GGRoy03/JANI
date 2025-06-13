#include "jani.h"

// WARN: Still unsure how to structure the code base.
#include "jani_internals.cpp"

namespace JANI
{

// ===========================================
// Text 
// ===========================================

static bool
IsLetter(u8 c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

static bool
LineStartsWith(u8* Line, const char* Pattern)
{
    while (*Pattern)
    {
        if (*Pattern++ != *Line++)
        {
            return false;
        }
    }

    return true;
}

jani_font_map*
LoadFontMap(jani_context *Context, const char* Path)
{
    jani_font_map            *Result     = (jani_font_map*)
        Context->Allocator.Allocate(sizeof(jani_font_map));
    jani_platform_read_result FileBuffer =
        Context->Platform.JaniReadFile(Path, &Context->Allocator);

    u32 At = 0;
    while(At < FileBuffer.ContentSize)
    {
        u8 *LineStart = FileBuffer.Content + At;

        if(LineStartsWith(LineStart, "common"))
        {
            u32 DummyBase;
            sscanf_s((const char*)LineStart, "common lineHeight=%u base=%u scaleW=%u scaleH=%u"
                    ,&Result->LineHeight, &DummyBase, &Result->TextureSizeX, &Result->TextureSizeY);
        }
        else if (LineStartsWith(LineStart, "chars"))
        {
            sscanf_s((const char*)LineStart, "chars count=%u", &Result->GlyphCount);
            size_t AllocSize = Result->GlyphCount * sizeof(jani_glyph);
            Result->Glyphs    = (jani_glyph*)Context->Allocator.Allocate(AllocSize);
        }
        else if(LineStartsWith(LineStart, "char"))
        {
            jani_glyph Glyph = {};
            sscanf_s((const char*)LineStart, "char id=%u x=%u y=%u width=%u height=%u xoffset=%u yoffset=%u xadvance=%u",
                     &Glyph.ID, &Glyph.PositionInAtlasX, &Glyph.PositionInAtlasY, &Glyph.SizeX, &Glyph.SizeY, &Glyph.OffsetX,
                     &Glyph.OffsetY, &Glyph.XAdvance);

            // BUG: This will fail if IDs are not sequential. Some map? 
            u32 GlyphIndex = Glyph.ID - ID_OFFSET;
            if(GlyphIndex >= 0)
            {
                Result->Glyphs[GlyphIndex] = Glyph;
            }
        }
        // NOTE: We skip kerning for now.

        while(FileBuffer.Content[At] != '\n')    At++;
        while(At < FileBuffer.ContentSize && !IsLetter(FileBuffer.Content[At])) At++;
    }

    Context->Allocator.Free(FileBuffer.Content, FileBuffer.ContentSize);

    Result->Loaded = true;
    return Result;
}

void
SetFontMap(jani_context *Context, jani_font_map *Map)
{
    if(Context && Map)
    {
        Context->ActiveFontMap = Map;
    }
}

// ===========================================
// Frame
// ===========================================

void
BeginUIFrame(jani_context *Context)
{
    if(!Context->Initialized)
    {
        Context->Allocator = jani_allocator(Megabytes(10));
        Context->Backend   = (jani_backend*)
            Context->Allocator.Allocate(sizeof(jani_backend));

        Context->Initialized = true;
    }

    JANI_PLATFORM::DoPlatformWorkBeforeFrame(Context);

}

// NOTE: The behavior of this must be somehow changed by the current pipeline..
void 
DrawBox(jani_context *Context)
{
    jani_draw_info Info  = {};
    Info.DrawType        = JANI_DRAW_QUAD;
    Info.VtxBufferTarget = 0;
    Info.IdxBufferTarget = 0;

    jani_quad_payload *Payload = &Info.Payload.Quad;
    Payload->SizeX             = 500;
    Payload->SizeY             = 500;
    Payload->TopLeftX          = 300;
    Payload->TopLeftY          = 300;

    jani_pipeline_handle CurrentHandle = Context->Backend->ActivePipeline;
    u16                  CurrentIndex  = GET_INDEX_FROM_HANDLE(CurrentHandle);
    jani_pipeline_state  *State        = Context->Backend->States + CurrentIndex;

    State->DrawList.VtxBuffer.FrameSize += 4 * State->InputStride;
    State->DrawList.IdxBuffer.FrameSize += 6 * sizeof(u32);
}

void
DrawBox(jani_context *Context, jani_pipeline_handle Handle)
{
    jani_draw_info Info  = {};
    Info.DrawType        = JANI_DRAW_QUAD;
    Info.VtxBufferTarget = 0;
    Info.IdxBufferTarget = 0;

    jani_quad_payload *Payload = &Info.Payload.Quad;
    Payload->SizeX             = 500;
    Payload->SizeY             = 500;
    Payload->TopLeftX          = 300;
    Payload->TopLeftY          = 300;

    u16                  Index  = GET_INDEX_FROM_HANDLE(Handle);
    jani_pipeline_state  *State = Context->Backend->States + Index;

    State->DrawList.VtxBuffer.FrameSize += 4 * State->InputStride;
    State->DrawList.IdxBuffer.FrameSize += 6 * sizeof(u32);

    State->DrawList.DrawInfos.Push(Info);
}

void
Text(jani_context *Context, const char* Text, jani_pipeline_handle Handle)
{
    u32 Length = StringLength(Text);

    // NOTE: For now we hardcode our pen position.
    f32 PenX = 500;
    f32 PenY = 400;

    jani_font_map *FontMap = Context->ActiveFontMap;

    u16                  PIndex = GET_INDEX_FROM_HANDLE(Handle);
    jani_pipeline_state  *State = Context->Backend->States + PIndex;

    for(u32 Index = 0; Index < Length; Index++)
    {
        char C = Text[Index];

        jani_glyph *G = FontMap->Glyphs + (C - 32);

        jani_draw_info Info  = {};
        Info.DrawType        = JANI_DRAW_QUAD;
        Info.VtxBufferTarget = 0;
        Info.IdxBufferTarget = 0;

        jani_quad_payload *Payload = &Info.Payload.Quad;
        Payload->SizeX             = (f32)G->SizeX;
        Payload->SizeY             = (f32)G->SizeY;
        Payload->TopLeftX          = PenX;
        Payload->TopLeftY          = PenY;

        PenX += G->XAdvance;

        State->DrawList.VtxBuffer.FrameSize += 4 * State->InputStride;
        State->DrawList.IdxBuffer.FrameSize += 6 * sizeof(u32);

        State->DrawList.DrawInfos.Push(Info);
    }
}

void
EndUIFrame(jani_context *Context)
{
    for(u32 Index = 0; Index < Context->Backend->PipelineBufferSize; Index++)
    {
        jani_pipeline_state *Pipeline = Context->Backend->States + Index;

        if(Pipeline->VertexArrayObject != 0)
        {
            PrepareDrawCommands(Context, Pipeline);
            DrawPipelineCommands(Pipeline);
        }
    }
}

}

