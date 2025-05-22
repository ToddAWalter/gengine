//
// Clark Kromenaker
//
// Encapsulates logic related to determining where a walker can walk in a scene,
// the path it should take, and any debug/rendering helpers.
//
#pragma once
#include <vector>
#include <unordered_set>

#include "Rect.h"
#include "Vector2.h"
#include "Vector3.h"

class Texture;

class WalkerBoundary
{
public:
	bool FindPath(const Vector3& fromWorldPos, const Vector3& toWorldPos, std::vector<Vector3>& outPath);
	Vector3 FindNearestWalkablePosition(const Vector3& worldPos) const;
	
	void SetTexture(Texture* texture) { mTexture = texture; }
	Texture* GetTexture() const { return mTexture; }
	
	void SetSize(const Vector2& size) { mSize = size; }
	Vector2 GetSize() const { return mSize; }
	
	void SetOffset(const Vector2& offset) { mOffset = offset; }
	Vector2 GetOffset() const { return mOffset; }

    void SetRegionBlocked(int regionIndex, int regionBoundaryIndex, bool blocked);
    int GetRegionIndex(const Vector3& worldPos);

    void SetUnwalkableRect(const std::string& name, const Rect& worldRect);
    void ClearUnwalkableRect(const std::string& name);
    void DrawUnwalkableRects();

private:
	// The texture provides vital data about walkable areas.
	// Each pixel correlates to a spot in the scene.
	// The pixel color indicates whether a spot is walkable and how walkable.
	Texture* mTexture = nullptr;
	
	// Size specifies scale of the walker bounds relative to the 3D scene.
	Vector2 mSize;
	
	// An offset for the bottom-left of the walker bounds from the world origin.
	Vector2 mOffset;

    // Regions that are not walkable.
    // By default 255 (black areas) are not walkable. Also disallow 9/8 as they are very close to the edges and look weird to walk there!
    std::unordered_set<int> mUnwalkableRegions = { 255, 9, 8 };

    // Rectangular areas that are blocked and unwalkable.
    std::vector<std::pair<std::string, Rect>> mUnwalkableRects;

    // The pathfinding grids are quite dense, and we can save some time by skipping over some nodes in the grid.
    // This value starts higher, but is cut in half when we can't find a path.
    int mPathfindingNodeSkip = 4;
	
	bool IsWorldPosWalkable(const Vector3& worldPos) const;
	bool IsTexturePosWalkable(const Vector2& texturePos) const;
	
	Vector2 WorldPosToTexturePos(const Vector3& worldPos) const;
	Vector3 TexturePosToWorldPos(Vector2 texturePos) const;

    int GetRegionForTexturePos(const Vector2& texturePos) const;
	
	Vector2 FindNearestWalkableTexturePosToWorldPos(const Vector3& worldPos) const;
    
    bool FindPathBFS(const Vector2& start, const Vector2& goal, std::vector<Vector2>& outPath, int graphFidelity = 1) const;
    bool FindPathAStar(const Vector2& start, const Vector2& goal, std::vector<Vector2>& outPath) const;
};
