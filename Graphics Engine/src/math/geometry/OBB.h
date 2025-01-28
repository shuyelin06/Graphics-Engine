#pragma once

#include "AABB.h"
#include "../Quaternion.h"
#include "../Vector3.h"

namespace Engine {
namespace Math {
// OrientedBoundingBox (OBB)
// Represents an OBB in 3D space, which is an AABB
// (ideally centered), which has been rotated 
// and translated.
class OBB {
private:
    AABB aabb;
    
    Vector3 center;
    Quaternion rotation;

public:
    OBB();
};

}
} // namespace Engine