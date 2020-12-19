//
// Debug.cpp
//
// Clark Kromenaker
//
#include "Debug.h"

#include "AABB.h"
#include "Material.h"
#include "Matrix4.h"
#include "Mesh.h"
#include "Plane.h"
#include "Rect.h"
#include "Services.h"
#include "Triangle.h"
#include "Vector3.h"

extern Mesh* line;
extern Mesh* axes;

std::list<DrawCommand> Debug::sDrawCommands;
Shader* Debug::sDrawShader = nullptr;

// Default debug settings.
bool Debug::sRenderActorTransformAxes = false;
bool Debug::sRenderSubmeshLocalAxes = false;
bool Debug::sRenderRectTransformRects = false;

void Debug::DrawLine(const Vector3& from, const Vector3& to, const Color32& color, float duration)
{
	// The line "starts at" the from position.
	// Scale the line to the end point with appropriate scale matrix.
	Matrix4 translateMatrix = Matrix4::MakeTranslate(from);
	Matrix4 scaleMatrix = Matrix4::MakeScale(to - from);
	
	// Combine in order (Scale, Rotate, Translate) to generate world transform matrix.
	DrawCommand command;
	command.mesh = line;
	command.worldTransformMatrix = translateMatrix * scaleMatrix;
	command.color = color;
	command.timer = duration;
	sDrawCommands.push_back(command);
}

void Debug::DrawAxes(const Vector3& position, float duration)
{
	DrawAxes(Matrix4::MakeTranslate(position), duration);
}

void Debug::DrawAxes(const Matrix4& worldTransform, float duration)
{
	DrawCommand command;
	command.mesh = axes;
	command.worldTransformMatrix = worldTransform;
	command.timer = duration;
	sDrawCommands.push_back(command);
}

void Debug::DrawRect(const Rect& rect, const Color32& color, float duration, const Matrix4* transformMatrix)
{
    // Corners of the rect.
	Vector2 bottomLeft = rect.GetMin();
	Vector2 topRight = rect.GetMax();
	Vector2 topLeft = Vector2(bottomLeft.x, topRight.y);
	Vector2 bottomRight = Vector2(topRight.x, bottomLeft.y);
	
    // May need to transform to world space before drawing.
    if(transformMatrix != nullptr)
    {
        bottomLeft = transformMatrix->TransformPoint(bottomLeft);
        topRight = transformMatrix->TransformPoint(topRight);
        topLeft = transformMatrix->TransformPoint(topLeft);
        bottomRight = transformMatrix->TransformPoint(bottomRight);
    }
    
    // Draw rect sides.
	DrawLine(bottomLeft, topLeft, color, duration);
	DrawLine(topLeft, topRight, color, duration);
	DrawLine(topRight, bottomRight, color, duration);
	DrawLine(bottomRight, bottomLeft, color, duration);
}

void Debug::DrawAABB(const AABB& aabb, const Color32& color, float duration, const Matrix4* transformMatrix)
{
    Vector3 min = aabb.GetMin();
    Vector3 max = aabb.GetMax();
    
    // Left side of box.
    Vector3 p0(min.x, min.y, min.z);
    Vector3 p1(min.x, min.y, max.z);
    Vector3 p3(min.x, max.y, min.z);
    Vector3 p2(min.x, max.y, max.z);
    
    // Right side of box.
    Vector3 p4(max.x, min.y, min.z);
    Vector3 p5(max.x, min.y, max.z);
    Vector3 p7(max.x, max.y, min.z);
    Vector3 p6(max.x, max.y, max.z);
    
    // May need to transform to world space before drawing.
    if(transformMatrix != nullptr)
    {
        p0 = transformMatrix->TransformPoint(p0);
        p1 = transformMatrix->TransformPoint(p1);
        p2 = transformMatrix->TransformPoint(p2);
        p3 = transformMatrix->TransformPoint(p3);
        p4 = transformMatrix->TransformPoint(p4);
        p5 = transformMatrix->TransformPoint(p5);
        p6 = transformMatrix->TransformPoint(p6);
        p7 = transformMatrix->TransformPoint(p7);
    }
    
    // Draw the lines of the box.
    DrawLine(p0, p1, color, duration);
    DrawLine(p1, p2, color, duration);
    DrawLine(p2, p3, color, duration);
    DrawLine(p3, p0, color, duration);
    
    DrawLine(p4, p5, color, duration);
    DrawLine(p5, p6, color, duration);
    DrawLine(p6, p7, color, duration);
    DrawLine(p7, p4, color, duration);
    
    DrawLine(p0, p4, color, duration);
    DrawLine(p1, p5, color, duration);
    DrawLine(p2, p6, color, duration);
    DrawLine(p3, p7, color, duration);
}

void Debug::DrawPlane(const Plane& plane, const Color32& color, float duration, const Matrix4* transformMatrix)
{
    // Get closest point to origin.
    Vector3 pointOnPlane = plane.GetClosestPoint(Vector3::Zero);
    
    // Draw a box at that point, then a short line indicating direction of the normal.
    DrawAABB(AABB(pointOnPlane, 4.0f, 4.0f, 4.0f), color, duration, transformMatrix);
    DrawLine(pointOnPlane, pointOnPlane + plane.normal * 10.0f, color, duration);
}

void Debug::DrawTriangle(const Triangle& triangle, const Color32& color, float duration, const Matrix4* transformMatrix)
{
    if(transformMatrix != nullptr)
    {
        Vector3 t0 = transformMatrix->TransformPoint(triangle.p0);
        Vector3 t1 = transformMatrix->TransformPoint(triangle.p1);
        Vector3 t2 = transformMatrix->TransformPoint(triangle.p2);
        
        Debug::DrawLine(t0, t1, color, duration);
        Debug::DrawLine(t1, t2, color, duration);
        Debug::DrawLine(t2, t0, color, duration);
    }
    else
    {
        Debug::DrawLine(triangle.p0, triangle.p1, color, duration);
        Debug::DrawLine(triangle.p1, triangle.p2, color, duration);
        Debug::DrawLine(triangle.p2, triangle.p0, color, duration);
    }
}

void Debug::Update(float deltaTime)
{
	// Decrement timers in all draw commands.
	for(auto& command : sDrawCommands)
	{
		command.timer -= deltaTime;
	}
	
	// Check for debug setting inputs.
	if(Services::GetInput()->IsKeyDown(SDL_SCANCODE_F1))
	{
		sRenderActorTransformAxes = !sRenderActorTransformAxes;
	}
	if(Services::GetInput()->IsKeyDown(SDL_SCANCODE_F2))
	{
		sRenderSubmeshLocalAxes = !sRenderSubmeshLocalAxes;
	}
	if(Services::GetInput()->IsKeyDown(SDL_SCANCODE_F3))
	{
		sRenderRectTransformRects = !sRenderRectTransformRects;
	}
}

void Debug::Render()
{
    if(sDrawShader == nullptr)
    {
        sDrawShader = Services::GetAssets()->LoadShader("3D-Color");
    }
    
	// We can just use any old material for now (uses default shader under the hood).
    Material material(sDrawShader);
	
	// Iterate over all draw commands and render them.
	auto it = sDrawCommands.begin();
	while(it != sDrawCommands.end())
	{
		DrawCommand& command = *it;
		
		// Set color and world transform.
		material.SetColor(command.color);
        material.Activate(command.worldTransformMatrix);
		
		// Draw the mesh.
		if(command.mesh != nullptr)
		{
			command.mesh->Render();
		}
		
		// Erase from list if time is up.
		if(command.timer <= 0.0f)
		{
			it = sDrawCommands.erase(it);
		}
		else
		{
			it++;
		}
	}
}
