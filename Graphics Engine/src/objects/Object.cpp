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
		vertex_buffer = { 0 };
		
		scale = Vector3(1, 1, 1);
		rotation = Vector3(0, 0, 0);
		position_local = Vector3(0, 0, 0);
	}

	/* --- Operations --- */
	// SetParent:
	// Sets the Object's parent
	void Object::setParent(Object* _parent)
	{
		parent = _parent;
	}

	// DistanceTo:
	// Returns the distance to another object
	float Object::distanceTo(Object* o)
	{
		return (o->getWorldPosition() - getWorldPosition()).magnitude();
	}

	// GetPosition:
	// Gets the Object's position
	Vector3 Object::getPosition() 
	{
		return position_local;
	}

	// GetWorldPosition:
	// Returns the Object's world position
	Vector3 Object::getWorldPosition()
	{
		return parent == nullptr ? position_local : position_local + parent->getWorldPosition();
	}

	// SetPosition:
	// Sets the Object's position
	void Object::setPosition(float x, float y, float z)
	{
		position_local.x = x;
		position_local.y = y;
		position_local.z = z;
	}

	void Object::setPosition(Vector3 position)
	{
		setPosition(position.x, position.y, position.z);
	}
	
	// OffsetPosition:
	// Offsets the local position
	void Object::offsetPosition(float x, float y, float z)
	{
		setPosition(position_local.x + x, position_local.y + y, position_local.z + z);
	}

	void Object::offsetPosition(Vector3 offset)
	{
		offsetPosition(offset.x, offset.y, offset.z);
	}

	// SetScale
	// Sets the Object's scale
	void Object::setScale(float scaleX, float scaleY, float scaleZ)
	{
		scale.x = scaleX;
		scale.y = scaleY;
		scale.z = scaleZ;
	}

	void Object::offsetScale(float offsetX, float offsetY, float offsetZ)
	{
		scale.x += offsetX;
		scale.y += offsetY;
		scale.z += offsetZ;
	}

	// SetRotation:
	// Sets the Object's rotation
	void Object::setRotation(float roll, float pitch, float yaw)
	{
		rotation.x = roll;
		rotation.y = pitch;
		rotation.z = yaw; 
	}

	// Offsets the rotation of the object
	void Object::offsetRotation(float offsetX, float offsetY, float offsetZ)
	{
		setRotation(rotation.x + offsetX, rotation.y + offsetY, rotation.z + offsetZ);
	}
	
	// LocalToWorldMatrix:
	// Returns the 4x4 matrix that, given a local point (like in a mesh), will
	// rotate and translate it to the world space as defined by 
	// the object's position and rotation.
	Matrix4 Object::localToWorldMatrix(void)
	{
		// Generate the transformation matrices
		Matrix4 m_scale = scaleMatrix();					// Scale
		Matrix4 m_rotation = rotationMatrix();				// Rotation
		Matrix4 m_translation = translationMatrix();		// Translation

		// Obtain parent's transformation matrix
		Matrix4 m_parent = parent == nullptr ? Matrix4::identity() : parent->localToWorldMatrix();	

		// Build final matrix
		// Left matrix gets precedence, as we are performing row-major multiplication
		return m_scale * m_rotation * m_translation * m_parent;
	}
	
	// Generates the object's scale matrix
	Matrix4 Object::scaleMatrix()
	{
		return Matrix4(scale.x, 0, 0, 0,
						0, scale.y, 0, 0,
						0, 0, scale.z, 0,
						0, 0, 0, 1);
	}

	// Generates the object's rotation matrix
	Matrix4 Object::rotationMatrix()
	{
		// Cache values to avoid recalculating sine and cosine a lot
		float cos_cache = 0;
		float sin_cache = 0;

		// Rotation about the x-axis (roll)
		cos_cache = cosf(rotation.x);
		sin_cache = sin(rotation.x);
		Matrix4 roll = Matrix4(
			1, 0, 0, 0,
			0, cos_cache, sin_cache, 0,
			0, -sin_cache, cos_cache, 0,
			0, 0, 0, 1
		);

		// Rotation about the y-axis (pitch)
		cos_cache = cosf(rotation.y);
		sin_cache = sin(rotation.y);
		Matrix4 pitch = Matrix4(
			cos_cache, 0, -sin_cache, 0,
			0, 1, 0, 0,
			sin_cache, 0, cos_cache, 0,
			0, 0, 0, 1
		);

		// Rotation about the z-axis (yaw)
		cos_cache = cosf(rotation.z);
		sin_cache = sin(rotation.z);
		Matrix4 yaw = Matrix4(
			cos_cache, sin_cache, 0, 0,
			-sin_cache, cos_cache, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		);

		return roll * pitch * yaw;
	}

	// Generates the object's translation matrix
	Matrix4 Object::translationMatrix()
	{
		return Matrix4(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			position_local.x, position_local.y, position_local.z, 1);
	}

	// SetVertexBuffer
	// Sets the vertex buffer
	void Object::setVertexBuffer(VertexBuffer _vertex_buffer)
	{
		vertex_buffer = _vertex_buffer;
	}

	// GetVertexBuffer
	// Returns the vertex buffer. Delegates the responsibility of instantiating 
	// this buffer to inheriting objects.
	VertexBuffer Object::getVertexBuffer(void)
	{
		return vertex_buffer;
	}
}
}