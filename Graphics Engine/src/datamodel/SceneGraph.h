#pragma once

namespace Engine
{
namespace Datamodel
{
    // Class SceneGraph:
    // Stores and manages all objects in the scene. Objects are stored in a tree-like hierarchy,
    // Parent <--> Children
    // Where all children node transforms are based off the local coordinate system of their parent.
    // The reference coordinate system of all nodes without a parent is the world coordinate system.
    class SceneGraph
    {
        
    };
}
}