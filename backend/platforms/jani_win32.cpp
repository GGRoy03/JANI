#include "jani.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace JANI_PLATFORM
{

jani_platform_read_result
Win32ReadFile(const char* Path, jani_allocator *Allocator)
{
    Jani_Assert(Path);

    jani_platform_read_result Result = {};

    HANDLE FileHandle = CreateFileA(Path, GENERIC_READ, FILE_SHARE_READ, NULL,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    Assert(FileHandle != INVALID_HANDLE_VALUE);

    struct _stat64 FileInfo;
    _stat64(Path, &FileInfo);

    Result.Content = (u8*)Allocator->Allocate(FileInfo.st_size);
    if (Result.Content)
    {
        DWORD BytesRead = 0;
        BOOL  ValidRead = ReadFile(FileHandle, Result.Content,
                                   (DWORD)FileInfo.st_size, &BytesRead, NULL);

        if (!ValidRead || BytesRead != FileInfo.st_size)
        {
            Allocator->Free(Result.Content, FileInfo.st_size);

            Result.Content     = NULL;
            Result.ContentSize = 0;
        }
        else
        {
            Result.ContentSize = BytesRead;
        }
    }

    CloseHandle(FileHandle);
    return Result;
}

void
Win32GetClientSize(void *WindowHandle, i32 *Width, i32 *Height)
{
    RECT Rect;
    GetClientRect((HWND)WindowHandle, &Rect);
    *Width  = Rect.right  - Rect.left;
    *Height = Rect.bottom - Rect.top;
}

void
DoPlatformWorkBeforeFrame(jani_context *Context)
{
    jani_platform *Platform = &Context->Platform;
    if(!Platform->Initialized)
    {
        Platform->WindowHandle = JANI_GLOBAL_WINDOW_HANDLE; 
        Platform->JaniReadFile = Win32ReadFile;

        Platform->Initialized = true;
    }

    Win32GetClientSize(Platform->WindowHandle,
                       &Context->ClientWidth, &Context->ClientHeight);
}

}
