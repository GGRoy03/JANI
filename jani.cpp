#include "jani.h"

namespace JANI
{

// ===========================================
// Text 
// ===========================================

static u32
StringLength(char* String)
{
    char* Start = String;
    while(*String) String++;
    return (u32)(String - Start);
}

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

jani_font_map
LoadFontMap(jani_context *Context, char* Path)
{
    jani_font_map             Result     = {};
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
                    ,&Result.LineHeight, &DummyBase, &Result.TextureSizeX, &Result.TextureSizeY);
        }
        else if (LineStartsWith(LineStart, "chars"))
        {
            sscanf_s((const char*)LineStart, "chars count=%u", &Result.GlyphCount);
            size_t AllocSize = Result.GlyphCount * sizeof(jani_glyph);
            Result.Glyphs    = (jani_glyph*)Context->Allocator.Allocate(AllocSize);
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
                Result.Glyphs[GlyphIndex] = Glyph;
            }
        }
        // NOTE: We skip kerning for now.

        while(FileBuffer.Content[At] != '\n')    At++;
        while(!IsLetter(FileBuffer.Content[At])) At++;
    }

    Context->Allocator.Free(FileBuffer.Content, FileBuffer.ContentSize);

    Result.Loaded = true;
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
        Context->Allocator    = jani_allocator(Megabytes(5));
        Context->CommandMetas = JaniBumper<jani_draw_meta>(8 * sizeof(jani_draw_meta));
        Context->Backend = (jani_backend*)
            Context->Allocator.Allocate(sizeof(jani_backend));

        Context->Initialized = true;
    }

    JANI_PLATFORM::DoPlatformWorkBeforeFrame(Context);

}

// NOTE: The behavior of this must be somehow changed by the current pipeline..
void 
DrawBox(jani_context *Context)
{
    jani_draw_meta Command = {};
    Command.PipelineHandle = Context->Backend->ActivePipeline;
    Command.Type           = JANI_DRAW_META_QUAD;

    draw_quad_payload Payload = {};
    Payload.SizeX             = 500;
    Payload.SizeY             = 500;
    Payload.TopLeftX          = 300;
    Payload.TopLeftY          = 300;
    Command.Payload.Quad      = Payload;

    Context->CommandMetas.Push(Command);

    // WARN: This might be done sowewhere else? Still unsure. Maybe append
    // the size to the command metadata?
    jani_pipeline_handle CurrentHandle = Context->Backend->ActivePipeline;
    u16                  CurrentIndex  = GET_INDEX_FROM_HANDLE(CurrentHandle);
    jani_pipeline_state  *State        = Context->Backend->States + CurrentIndex;

    State->FrameVertexSize += 4 * State->InputStride;
    State->FrameIndexSize  += 6 * sizeof(u32);
}

void
DrawText(jani_context *Context, char* Text)
{
    jani_draw_meta Meta = {};
    Meta.PipelineHandle = Context->Backend->ActivePipeline;
    Meta.Type           = JANI_DRAW_META_TEXT;

    draw_text_payload Payload = {};
    Payload.Text              = Text;
    Payload.Length            = StringLength(Text);

    Context->CommandMetas.Push(Meta);

    jani_pipeline_handle CurrentHandle = Context->Backend->ActivePipeline;
    u16                  CurrentIndex  = GET_INDEX_FROM_HANDLE(CurrentHandle);
    jani_pipeline_state  *State        = Context->Backend->States + CurrentIndex;

    State->FrameVertexSize += Payload.Length * 4 * State->InputStride;
}

void
EndUIFrame(jani_context *Context)
{
    sorted_metas Sorted   = SortDraws(Context);
    JaniBumper   Commands = BuildAndCopyData(Context, &Sorted);
    ExecuteDrawCommands(&Commands);

    // Clean-up
    Context->CommandMetas.Reset();
}

}

