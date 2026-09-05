#pragma once

#define ENABLE_RENDER_DOC

namespace RenderDoc
{
// Call this at the top of the application before initializing any graphics device.
void InitializeRenderDoc();
bool IsRenderDocInitialized();
void StartRenderDocCapture();
void EndRenderDocCaptureIfCapturing();
} // namespace RenderDoc