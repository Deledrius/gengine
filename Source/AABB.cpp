//
// AABB.cpp
//
// Clark Kromenaker
//
#include "AABB.h"

AABB::AABB(Vector3 min, Vector3 max)
{
	mMin = min;
	mMax = max;
}

/*
AABB::AABB(Vector3 center, float extentX, float extentY, float extentZ)
{
	Vector3 extent(extentX, extentY, extentZ);
	mMin = center - extent;
	mMax = center + extent;
}
*/

bool AABB::ContainsPoint(const Vector3& point) const
{
	// Point should be greater than min and less than max.
	return point.x >= mMin.x && point.y >= mMin.y && point.z >= mMin.z &&
		   point.x <= mMax.x && point.y <= mMax.y && point.z <= mMax.z;
}

Vector3 AABB::GetClosestPoint(const Vector3& point) const
{
	// Start with result as point.
	// If point is inside AABB, we just return the ummodified point in the end.
	Vector3 result = point;
	
	// Clamp point between min/max on x-axis.
	if(point.x > mMax.x)
	{
		result.x = mMax.x;
	}
	else if(point.x < mMin.x)
	{
		result.x = mMin.x;
	}
	
	// Clamp point between min/max on y-axis.
	if(point.y > mMax.y)
	{
		result.y = mMax.y;
	}
	else if(point.y < mMin.y)
	{
		result.y = mMin.y;
	}
	
	// Clamp point between min/max on z-axis.
	if(point.z > mMax.z)
	{
		result.z = mMax.z;
	}
	else if(point.z < mMin.z)
	{
		result.z = mMin.z;
	}
	return result;
}

