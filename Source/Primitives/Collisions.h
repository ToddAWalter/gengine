//
// Clark Kromenaker
//
// A static class to easily access collision checks between a variety of primitives.
//
#pragma once

#include <cfloat>
#include <string>

class Actor;
class AABB;
class LineSegment;
class Plane;
class Ray;
class Sphere;
class Triangle;
class Vector3;

struct RaycastHit
{
	// The "t" value at which the hit occurred.
	float t = FLT_MAX;
	
	// A name/identifier for the thing hit.
	std::string name;
	
	// An actor hit.
	Actor* actor = nullptr;
};

class Collisions
{
public:
	// Sphere
	static bool TestSphereSphere(const Sphere& s1, const Sphere& s2);
	static bool TestSphereAABB(const Sphere& s, const AABB& aabb);
	static bool TestSpherePlane(const Sphere& s, const Plane& p);
	static bool TestSphereTriangle(const Sphere& s, const Triangle& t, Vector3& intersection);
	
	// AABB
	static bool TestAABBAABB(const AABB& aabb1, const AABB& aabb2);
	//AABBPlane
	
	// Plane
	static bool TestPlanePlane(const Plane& p1, const Plane& p2);
	
	// Ray
	static bool TestRaySphere(const Ray& r, const Sphere& s);
	static bool TestRayAABB(const Ray& r, const AABB& aabb, RaycastHit& outHitInfo);
	static bool TestRayPlane(const Ray& r, const Plane& p);
	static bool TestRayTriangle(const Ray& r, const Triangle& t, RaycastHit& hitInfo);
	static bool TestRayTriangle(const Ray& r, const Vector3& p0, const Vector3& p1, const Vector3& p2, RaycastHit& outHitInfo);
	
	// Line Segment
	static bool TestLineSegmentSphere(const LineSegment& ls, const Sphere& s);
	static bool TestLineSegmentAABB(const LineSegment& ls, const AABB& aabb);
	static bool TestLineSegmentPlane(const LineSegment& ls, const Plane& p);
};
