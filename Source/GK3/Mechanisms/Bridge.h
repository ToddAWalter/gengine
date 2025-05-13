//
// Clark Kromenaker
//
// The invisible bridge used near the very end of the game.
// Gabe must cross a bridge where the floor keeps vanishing and reappearing.
//
#pragma once
#include "Actor.h"

#include "Animator.h"
#include "Vector2.h"

class GKActor;

class Bridge : public Actor
{
public:
    Bridge();

protected:
    void OnUpdate(float deltaTime) override;

private:
    // Pointer to Gabe/Ego actor.
    GKActor* mGabeActor = nullptr;

    // True after Gabe has approached the bridge one time.
    bool mGabeStartedPuzzle = false;

    // Index of the tile Gabe is currently standing on. -1 if not on any tile.
    int mGabeTileIndex = -1;

    // True if a jump is being performed.
    bool mJumping = false;

    // During a jump, index of the tile being jumped to.
    int mJumpTileIndex = -1;

    // A tile in the bridge puzzle.
    enum class TileState
    {
        Off,        // Tile is completely off and unused
        Glinting,   // Tile is playing Glint anim, can be jumped to
        Glowing,    // Tile is glowing, Gabe is standing on it
        Sleeping    // Tile was on, but has temporarily disappeared - if standing on it, you die!
    };
    struct Tile
    {
        // The actor associated with this tile.
        Actor* tileActor = nullptr;

        // The tiles are laid out on a 3x11 grid across the chasm.
        // This is the tile's position on that grid, where (0, 0) is at bottom-right (near Gabe's spawn point).
        Vector2 gridPos;

        // The tile's current state.
        TileState state = TileState::Off;

        // A timer for the current state.
        float stateTimer = 0.0f;

        // Animations for glowing/glinting this tile.
        Animation* glintAnim = nullptr;
        Animation* glowAnim = nullptr;
    };
    static const int kTileCount = 9;
    Tile mTiles[kTileCount];

    // Reusable anim params, used for jump anims.
    AnimParams mJumpAnimParams;

    void Die(bool duringJump);
    void ResetOnDeath();

    void SetTilePosition(int index, int x, int y);
    void GlintTile(int index);
    void GlowTile(int index);
    void SleepTile(int index, float sleepTime);
    void StartTilePattern();
    void JumpToTile(int index);

    void UpdateInteract();
};