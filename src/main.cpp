//==============================================================================
// file: main.cpp
// auth: Victor Schappert
// date: 20130618
// desc: DLL entry-point for JSuneido DLL interface
//==============================================================================

#include "jsdi_windows.h"

extern "C"
{

// DLL ENTRY-POINT. This function is called by the DLL loader when it loads
//     or unloads a DLL. The loader serializes calls to DllMain so that only a
//     single DllMain ever runs at the same time.
//         See http://msdn.microsoft.com/library/en-us/dllproc/base/dllmain.asp.
BOOL APIENTRY DllMain(
    HANDLE hModule,
    DWORD fdwReason,
    LPVOID lpvReserved
)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // A process is loading the DLL.
            break;
        case DLL_THREAD_ATTACH:
            // A process is creating a new thread.
            break;
        case DLL_THREAD_DETACH:
            // A thread exits normally.
            break;
        case DLL_PROCESS_DETACH:
            // A process unloads the DLL.
            break;
    }
    return TRUE;
}

} // extern "C"
