#include "Object.h"

#include <math.h>

namespace Engine
{
using namespace Math;

namespace Datamodel
{
	/* --- Constructors --- */
	// Constructor:
	// Creates an object with no parent and a
	// local position of (0,0,0)
	Object::Object()
	{
		parent = nullptr;
		mesh = nullptr;
	}

	/* --- Accessors --- */
	// GetParent:
	// Returns the object's parent
	Object* Object::getParent() const
	{
		return parent;
	}

	// GetTransform:
	// Returns the object's transform property
	Transform& Object::getTransform()
	{
		return transform;
	}
	
	// GetVelocity:
	// Returns the object's velocity
	Vector3& Object::getVelocity()
	{
		return velocity;
	}

	// GetAcceleration:
	// Returns the object's acceleration
	Vector3& Object::getAcceleration()
	{
		return acceleration;
	}

	// GetMesh:
	// Returns the object's renderable mesh
	Mesh* Object::getMesh() const
	{
		return mesh;
	}

	// SetParent:
	// Sets the Object's parent
	void Object::setParent(Object* _parent)
	{
		parent = _parent;
	}

	// SetMesh
	// Sets the object's mesh
	void Object::setMesh(Mesh* _mesh)
	{
		mesh = _mesh;
	}

	// LocalToWorldMatrix:
	// Returns an object's LocalToWorld transformation matrix
	Matrix4 Object::localToWorldMatrix() const
	{
		// Get local transformation matrix 
        Matrix4 m_local = transform.transformMatrix();

        // If parent exists, get parent transformation matrix;
        Matrix4 m_parent = (parent == nullptr) ? Matrix4::identity() : parent->localToWorldMatrix();

		// Build final matrix - multiplication is in order of left first to right
        // (row-major multiplication)
        return m_local * m_parent;
	}
}
}