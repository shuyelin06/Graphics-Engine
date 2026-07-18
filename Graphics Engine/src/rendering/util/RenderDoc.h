#pragma once

namespace Engine {
namespace Graphics {
namespace RenderDoc {
void InitializeRenderDoc();
bool IsRenderDocInitialized();
void StartRenderDocCapture();
void EndRenderDocCaptureIfCapturing();
} // namespace RenderDoc
} // namespace Graphics
} // namespace Engine