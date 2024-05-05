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
		// Objects start with no parent and no children
		parent = nullptr;
		children = vector<Object*>(0);
		
		// Default transform
		transform = Transform();

		// Objects are by default not renderable
		mesh = nullptr;

		// No velocity or acceleration
		velocity = Vector3();
		acceleration = Vector3();
	}

	// Destructor:
	// Frees all children of the object
	Object::~Object()
	{
		for (Object* child : children)
			delete child;
	}

	/* --- Accessors --- */
	// GetParent:
	// Returns the object's parent
	Object* Object::getParent() const
	{
		return parent;
	}

	// GetChildren:
	// Returns the object's children
	vector<Object*>& Object::getChildren()
	{
		return children;
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