#pragma once

namespace Engine {
namespace Math {
// Color Class:
// Represents an RGB color, where each component is in the range [0,1].
class Color {
  public:
    float r, g, b;

    Color();
    Color(float r, float g, float b);

    ~Color();

    // Commonly used colors
    static Color White();
    static Color Red();
    static Color Green();
    static Color Blue();
};
} // namespace Math
} // namespace Engine