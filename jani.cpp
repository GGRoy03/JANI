#include "jani.h"

namespace JANI
{

// ===========================================
// Frame
// ===========================================

void
BeginUIFrame(jani_context *Context)
{
    if(!Context->Initialized)
    {
        Context->CommandMetas = JaniBumper<jani_draw_meta>(8 * sizeof(jani_draw_meta));
        Context->Backend = (jani_backend*)
            Context->Allocator.Allocate(sizeof(jani_backend));

        Context->Initialized = true;
    }
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
}

void
EndUIFrame(jani_context *Context)
{
    sorted_metas Sorted   = SortDraws(Context);
    JaniBumper   Commands = BuildAndCopyData(Context->Backend, &Sorted);
    ExecuteDrawCommands(&Commands);

    // Clean-up
    Context->CommandMetas.Reset();
}

}

