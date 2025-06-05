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
        // TODO: Set the defaults
        Context->Commands = JaniBumper<jani_draw_command>(8*sizeof(jani_draw_command));
        Context->Initialized = true;
    }

}

void
EndUIFrame(jani_context *Context)
{
    // Prep end of frame

    // Draw calls

    // Clean-up
    Context->Commands.Reset();
}

}
