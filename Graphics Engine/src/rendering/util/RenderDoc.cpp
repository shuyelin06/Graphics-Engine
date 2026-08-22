#include "RenderDoc.h"

// Comment this in to enable RenderDoc
#define ENABLE_RENDER_DOC

#ifdef ENABLE_RENDER_DOC
#include "renderdoc/renderdoc_app.h"
#include <windows.h>

// API Handle
static RENDERDOC_API_1_7_0* rdoc_api = NULL;
constexpr const char* RenderDocDLL = "renderdoc.dll";
constexpr const char* RenderDocDLLPath =
    "C:\\Program Files\\RenderDoc\\renderdoc.dll";
#endif

namespace Engine
{
namespace Graphics
{
namespace RenderDoc
{
void InitializeRenderDoc()
{
#ifdef ENABLE_RENDER_DOC
    // Attempt to fetch the renderdoc DLL if it is already loaded
    // If it doesn't exist, try to load the DLL
    HMODULE mod = GetModuleHandleA(RenderDocDLL);

    if (!mod)
    {
        mod = LoadLibraryA(RenderDocDLLPath);
    }

    if (mod)
    {
        // DLL load success. Attempt to fetch the function that we do RenderDoc
        // API calls from
        pRENDERDOC_GetAPI RENDERDOC_GetAPI =
            (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");

        // Function fetch success. Attempt to fetch the API entrypoint and save
        // to rdoc_api
        if (RENDERDOC_GetAPI)
        {
            int result = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_7_0,
                                          (void**)&rdoc_api);
            if (result != 1)
            {
                rdoc_api = nullptr;
            }
        }
    }

    // Set Configurations
    if (rdoc_api)
    {
        // Turn off all overlay text
        rdoc_api->MaskOverlayBits(0, 0);
    }
#endif
}
bool IsRenderDocInitialized()
{
#ifdef ENABLE_RENDER_DOC
    return rdoc_api != nullptr;
#else
    return false;
#endif
}
void StartRenderDocCapture()
{
#ifdef ENABLE_RENDER_DOC
    if (rdoc_api)
        rdoc_api->StartFrameCapture(nullptr, nullptr);
#endif
}
void EndRenderDocCaptureIfCapturing()
{
#ifdef ENABLE_RENDER_DOC
    if (rdoc_api && rdoc_api->IsFrameCapturing())
    {
        rdoc_api->EndFrameCapture(nullptr, nullptr);

        // Launch the RenderDoc UI to view the capture
        // immediately
        if (!rdoc_api->IsTargetControlConnected())
        {
            rdoc_api->LaunchReplayUI(1, "");
        }
    }
#endif
}

} // namespace RenderDoc
} // namespace Graphics
} // namespace Engine