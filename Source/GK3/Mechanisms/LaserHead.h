//
// Clark Kromenaker
//
// The "Laser Heads" are 5 busts/statues in the CS2 location.
// They are used for a puzzle where the player needs to make the shape of a pentagram.
//
// This component encompasses the logic for positioning the heads, toggling their lasers,
// and turning the heads left or right to create the shape.
//
#pragma once
#include "Component.h"

#include <functional>

class GKObject;

class LaserHead : public Component
{
    TYPEINFO_SUB(LaserHead, Component);
public:
    LaserHead(Actor* owner, int index);

    void SetLaserEnabled(bool enabled);
    bool IsLaserEnabled() const;

    void TurnLeft(const std::function<void()>& callback);
    void TurnRight(const std::function<void()>& callback);

protected:
    void OnUpdate(float deltaTime) override;

private:
    // The child laser object attached to the head.
    GKObject* mLaser = nullptr;

    // This head's index. Each head in the room has an index 0-4.
    int mIndex = 0;

    // This head's turn index. Each head can turn to 5 different angles.
    // This lets us determine the angle the head should currently be at.
    int mTurnIndex = 2;

    // Tracks the turn of the head via a lerp. Longer durations means slower turn speed.
    const float kTurnDuration = 2.0f;
    float mTurnTimer = 0.0f;

    void Turn(int dir, const std::function<void()>& callback);
};