//
// WalkerBoundary.cpp
//
// Clark Kromenaker
//
#include "WalkerBoundary.h"

#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "GMath.h"
#include "Texture.h"

struct NodeInfo
{
	Vector2 parent;
	float h;
	float g;
	float GetF() { return h + g; }
};

bool WalkerBoundary::FindPath(Vector3 from, Vector3 to, std::vector<Vector3>& outPath) const
{
	// Make sure path vector is empty.
	outPath.clear();
	
	// Pick goal position. If "to" is walkable, we can use it directly.
    // Otherwise, find the nearest walkable position to "to".
	Vector2 goal;
	if(IsWorldPosWalkable(to))
	{
		goal = WorldPosToTexturePos(to);
	}
	else
	{
		// If "to" is not walkable, we need to find nearest walkable position as our goal.
		goal = FindNearestWalkableTexturePosToWorldPos(to);
	}
    
	// Pick start position. If "from" is walkable, we can use it directly.
	Vector2 start;
	if(IsWorldPosWalkable(from))
	{
		start = WorldPosToTexturePos(from);
	}
	else
	{
		// If "from" is not walkable, find nearest walkable and use that instead.
		// Walker will move from current (unwalkable) position to this position when it starts walking.
		start = FindNearestWalkableTexturePosToWorldPos(from);
	}
	
	// Let's try some A* to figure this out...
	std::vector<Vector2> openSet;
	std::unordered_set<Vector2, Vector2Hash> closedSet;
	std::unordered_map<Vector2, NodeInfo, Vector2Hash> infos;
	
	// Start with goal and put it in closed set.
	Vector2 current = start;
	closedSet.insert(current);
	
	// Iterate until we find the start node.
	while(current != goal)
	{
		// Create neighbors array - including diagonals!
		Vector2 neighbors[8];
		neighbors[0] = current + Vector2::UnitY;
		neighbors[1] = current - Vector2::UnitY;
		neighbors[2] = current + Vector2::UnitX;
		neighbors[3] = current - Vector2::UnitX;
		
		neighbors[4] = current + Vector2(1, 1);
		neighbors[5] = current + Vector2(1, -1);
		neighbors[6] = current + Vector2(-1, 1);
		neighbors[7] = current + Vector2(-1, -1);
		
		// See if we should add neighbors to open set.
		for(auto& neighbor : neighbors)
		{
			// Ignore any neighbor that has pixel color black (not walkable).
            int index = mTexture->GetPaletteIndex(neighbor.x, neighbor.y);
			if(index == 255) { continue; }
			
			// As a foundation, using Euclidean distance for the heuristic is pretty safe.
            // But as a modifier , multiplying by index (greater index means higher cost) seems to give OK results.
            float edgeCost = (neighbor - current).GetLength() * index;
			
			// Ignore anything already in the closed set.
			if(closedSet.find(neighbor) != closedSet.end())
			{
				continue;
			}
			else if(std::find(openSet.begin(), openSet.end(), neighbor) != openSet.end())
			{
				// If in the open set, check for adoption.
				// If lower g value, reparent to current.
				float newG = infos[current].g + edgeCost;
				if(newG < infos[neighbor].g)
				{
					infos[neighbor].parent = current;
					infos[neighbor].g = newG;
				}
			}
			else
			{
				// Found a new node - create an info for it and add to open set.
				NodeInfo nodeInfo;
				nodeInfo.parent = current;
				
				nodeInfo.h = (goal - neighbor).GetLength() * index;
				//nodeInfo.h = 0.0f; // No heuristic? (Dijkstra)
				nodeInfo.g = infos[current].g + edgeCost;
				infos[neighbor] = nodeInfo;
				openSet.push_back(neighbor);
			}
		}
		
		// Could not find a path.
		if(openSet.empty())
		{
			return false;
		}
		
		// Find open set item with lowest f value.
		std::vector<Vector2>::iterator nextIt = openSet.begin();
		float lowestF = infos[*nextIt].GetF();
		for(auto it = openSet.begin() + 1; it != openSet.end(); it++)
		{
			NodeInfo nodeInfo = infos[*it];
			if(nodeInfo.GetF() < lowestF)
			{
				nextIt = it;
				lowestF = nodeInfo.GetF();
			}
		}
		
		// This'll be the next node - remove from open set, put in closed set.
		current = *nextIt;
		openSet.erase(nextIt);
		closedSet.insert(current);
	}
    
	// Iterate back to start, pushing world position of each node onto our path.
    // This leaves the path with start node at back, goal node at front - caller can traverse back-to-front.
	while(current != start)
	{
		outPath.push_back(TexturePosToWorldPos(current));
		current = infos[current].parent;
	}
	return true;
}

Vector3 WalkerBoundary::FindNearestWalkablePosition(const Vector3& position) const
{
	// Easy case: the position provided is already walkable.
	if(IsWorldPosWalkable(position)) { return position; }
	
	// Find nearest walkable position on texture, convert to world space.
	Vector2 walkableTexturePos = FindNearestWalkableTexturePosToWorldPos(position);
	return TexturePosToWorldPos(walkableTexturePos);
}

bool WalkerBoundary::IsWorldPosWalkable(Vector3 worldPos) const
{
	// Convert to texture position and check that.
	return IsTexturePosWalkable(WorldPosToTexturePos(worldPos));
}

bool WalkerBoundary::IsTexturePosWalkable(Vector2 texturePos) const
{
	// If no texture...can walk anywhere?
	if(mTexture == nullptr) { return true; }
	
	// The color of the pixel at pos seems to indicate whether that spot is walkable.
	// White = totally OK to walk 				(255, 255, 255)
	// Blue = OK to walk						(0, 0, 255)
	// Green = sort of OK to walk 				(0, 255, 0)
	// Red = getting less OK to walk 			(255, 0, 0)
	// Yellow = sort of not OK to walk 			(255, 255, 0)
	// Magenta = really pushing it here 		(255, 0, 255)
	// Grey = pretty not OK to walk here 		(128, 128, 128)
	// Cyan = this is your last warning, buddy 	(0, 255, 255)
	// Black = totally not OK to walk 			(0, 0, 0)
	Color32 color = mTexture->GetPixelColor32(texturePos.x, texturePos.y);
    //unsigned char index = mTexture->GetPaletteIndex(texturePos.x, texturePos.y);
    //std::cout << (int)index << ", " << color << std::endl;
    
	// Basically, if the texture color is not black, you can walk there.
	return color != Color32::Black;
}

Vector2 WalkerBoundary::WorldPosToTexturePos(Vector3 worldPos) const
{
	// If no texture, the end result is going to be zero.
	if(mTexture == nullptr) { return Vector2::Zero; }
    
	// Add walker boundary's world position offset.
	// This causes the position to be relative to the texture's origin (lower left) instead of the world origin.
	Vector2 texturePos;
	texturePos.x = worldPos.x + mOffset.x;
	texturePos.y = worldPos.z + mOffset.y;
	//std::cout << "Offset Pos: " << position << std::endl;
	
	// Divide position by walkable area size to get a normalized position within that area.
	// Hopefully 0-1, but could be outside those bounds. If so, not walkable.
	texturePos.x = texturePos.x / mSize.x;
	texturePos.y = texturePos.y / mSize.y;
	//std::cout << "Normalized Pos: " << position << std::endl;
	
	// Multiply by texture width/height to determine the pixel within the texture.
	texturePos.x = texturePos.x * mTexture->GetWidth();
	texturePos.y = texturePos.y * mTexture->GetHeight();
	//std::cout << "Pixel Pos: " << position << std::endl;
	
	// Need to flip Y because the calculated value is from lower-left of the walkable area.
	// But texture sample X/Y are from upper-left.
	texturePos.y = mTexture->GetHeight() - texturePos.y;
	
	// Texture positions are integers.
	texturePos.x = (int)texturePos.x;
	texturePos.y = (int)texturePos.y;
	return texturePos;
}

Vector3 WalkerBoundary::TexturePosToWorldPos(Vector2 texturePos) const
{
	// If no texture, the end result is going to be zero.
	if(mTexture == nullptr) { return Vector3::Zero; }
    
    // A texture pos actually correlates to the bottom-left corner of the pixel.
    // But we want center of pixel...so let's offset before the conversion!
    texturePos.x = texturePos.x + 0.5f;
    texturePos.y = texturePos.y + 0.5f;
    
	// Flip y because texture pos is from top-left, but we need lower-left for world pos conversion.
	texturePos.y = mTexture->GetHeight() - texturePos.y;
    
	// Divide by texture width/height to get normalized position within the texture (0-1).
	Vector3 worldPos;
	worldPos.x = texturePos.x / mTexture->GetWidth();
	worldPos.z = texturePos.y / mTexture->GetHeight();
	
	// Multiply by size to get unit in world space.
	worldPos.x = worldPos.x * mSize.x;
	worldPos.z = worldPos.z * mSize.y;
	
	// Subtract offset to go from "texture space" to "world space".
	worldPos.x = worldPos.x - mOffset.x;
	worldPos.z = worldPos.z - mOffset.y;
	return worldPos;
}

Vector2 WalkerBoundary::FindNearestWalkableTexturePosToWorldPos(const Vector3& worldPos) const
{
	// We need a texture.
	if(mTexture == nullptr) { return Vector2::Zero; }
	
	// If the passed in position is already walkable, just return that position in texture space.
	if(IsWorldPosWalkable(worldPos))
	{
		return WorldPosToTexturePos(worldPos);
	}
	
	// Convert target position to texture position.
	Vector2 targetTexturePos = WorldPosToTexturePos(worldPos);
	
	// Let's just brute force this for now - search O(n^2) for the nearest walkable position.
	// This can probably be more efficient based on whether target is to left/right/above/below/inside the texture.
	// But these walker boundary textures are really small, and this doesn't get called often, so this might work fine.
	Vector2 nearestWalkableTexturePos;
	float nearestDistanceSq = 9999.0f;
	for(int x = 0; x < mTexture->GetWidth(); ++x)
	{
		for(int y = 0; y < mTexture->GetHeight(); ++y)
		{
			Vector2 pos(x, y);
			if(IsTexturePosWalkable(pos))
			{
				float distSq = (pos - targetTexturePos).GetLengthSq();
				if(distSq < nearestDistanceSq)
				{
					nearestWalkableTexturePos = pos;
					nearestDistanceSq = distSq;
				}
			}
		}
	}
	return nearestWalkableTexturePos;
}
