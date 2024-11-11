#pragma once

#include "math/Transform.h"
#include "math/Matrix4.h"

namespace Engine
{
using namespace Math;

namespace Graphics {
    // Camera Class:
    // Represents the scene's camera, where everything
    // on the screen is rendered from the camera's point of view. 
    // Unless otherwise rotated, the camera's default view
    // is in the +Z axis.
    class Camera
    {
    protected:
        // Field of view
        float fov;

        // Z-near and z-far viewing planes
        float z_near;
        float z_far;

        // Transform
        Transform transform;

    public:
        Camera();
        ~Camera();

        // Get the camera's attributes
        const Transform& getTransform() const;
        Transform& getTransform();

        float getFOV() const;
        float getZNear() const;
        float getZFar() const;

        // Set the camera's attributes
        void setFOV(float new_fov);
        void setZNear(float new_znear);
        void setZFar(float new_zfar);

        // World -> Camera Matrix
        const Matrix4 getWorldToCameraMatrix(void) const;

        // Camera -> Projected Space Matrix
        const Matrix4 getProjectionMatrix(void) const;

    };
}
}