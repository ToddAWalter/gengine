//
// Clark Kromenaker
//
// Camera used to actually play the game. Obeys all game world laws.
//
// Because GK3 is technically a "3rd person" game and the player directly controls the camera,
// you can probably consider this to be the "player actor" or "player controller". So, most user inputs are checked here.
//
#pragma once
#include "Actor.h"

#include <functional>
#include <vector>

#include "Ray.h"

class Camera;
class GKObject;
class Model;
class OptionBar;

class GameCamera : public Actor
{
public:
    static void SetCameraGlideEnabled(bool enabled);
    static bool IsCameraGlideEnabled();

    static void SetCinematicsEnabled(bool enabled);
    static bool AreCinematicsEnabled();

    GameCamera();

    void AddBounds(Model* model) { mBoundsModels.push_back(model); }
    void RemoveBounds(Model* model);
    void SetBoundsEnabled(bool enabled) { mBoundsEnabled = enabled; }

    void SetAngle(const Vector2& angle);
    void SetAngle(float yaw, float pitch);
    Vector2 GetAngle() const;

    void SetForcedCinematicMode(bool forced) { mForcedCinematicMode = forced; }
    bool IsForcedCinematicMode() const { return mForcedCinematicMode; }

    void Glide(const Vector3& position, const Vector2& angle, std::function<void()> callback);

    void Inspect(const std::string& noun, const Vector3& position, const Vector2& angle, const std::function<void()>& callback);
    void Uninspect(const std::function<void()>& callback);
    const std::string& GetInspectNoun() const { return mInspectNoun; }

    GKObject* RaycastIntoScene(bool interactiveOnly);
    Ray GetSceneRayAtMousePos();
    bool IsSceneInteractAllowed() const;
    void SetSceneInteractEnabled(bool enabled) { mSceneInteractEnabled = enabled; }
    void SetSceneActive(bool active) { mSceneActive = active; }
    void SetIgnoreFloor(bool ignoreFloor) { mIgnoreFloor = true; }

    Camera* GetCamera() { return mCamera; }

    void SaveFov();
    void RestoreFov();

protected:
    void OnUpdate(float deltaTime) override;

private:
    // Reference to underlying camera component.
    Camera* mCamera = nullptr;

    // Option bar - used to change settings, quit game, etc.
    OptionBar* mOptionBar = nullptr;

    // The last object hovered over. Used for toggling cursor highlight color.
    std::string mLastHoveredNoun;

    // If true, scene is "active", so perform scene updates (e.g. camera movement).
    // When scene is not active, game camera still handles some user input stuff.
    bool mSceneActive = true;

    // In forced cinematic mode, the camera has two special behaviors:
    // 1) Even if cinematics are disabled, the camera will obey "cut to camera angle" sheep calls.
    // 2) All movement or rotation of the camera is disabled.
    bool mForcedCinematicMode = false;

    // Cutscenes sometimes set the camera FOV, but forget to set it back afterwards.
    // To combat this, we remember the FOV pre-action and revert it afterwards.
    // (After some testing, even the original game does this!)
    float mSavedCameraFOV = 0.0f;

    // If true, normal raycasting and interacting logic with the scene is enabled and allowed.
    bool mSceneInteractEnabled = true;

    //////////////////
    // MOVEMENT
    //////////////////
    // Camera movement speed in units/second.
    // A modifier key (alt) causes the camera to move even faster.
    // These values were derived from trial and error!
    const float kSpeed = 500.0f;
    const float kFastSpeedMultiplier = 2.0f;

    // Camera rotation speed in radians/second. Again, trial and error!
    // This is about half the speed of the original game (from my observations). But I think the original game turns way too fast though!
    const float kRotationSpeed = Math::kPi;

    // How much you have to move the mouse (in pixels) to go as fast as keyboard keys. Derived from trial and error.
    // Smaller values make mouse movement more sensitive and speedier.
    const float kMouseRangePixels = 12.0f;

    // How far you need to move the mouse (in pixels) from click start pos to enable mouse-based movement.
    // Again...trial and error.
    const float kMouseMoveDistSq = 25.0f;

    // For collision, the camera is represented as a sphere. This is the sphere's radius.
    // This value was derived from trial & error.
    const float kCameraColliderRadius = 16.0f;

    // The camera's current yaw and pitch.
    // This is tracked separately from the transform mainly to enable clamping of pitch.
    float mYaw = 0.0f;
    float mPitch = 0.0f;

    // When mouse is clicked, this is start position of the click.
    // Used to determine when to enable mouse-based camera movement.
    Vector2 mClickStartPos;
    bool mStartedClickForMouseLock = false;

    // Track if current mouse inputs were used for mouse lock. If so, must release all mouse buttons before doing other mouse actions.
    // This is helpful/necessary to avoid accidentally opening option bar when releasing mouse buttons after mouse lock.
    bool mUsedMouseInputsForMouseLock = false;

    // Height of camera above ground. Always try to maintain some height above ground.
    // So, if moving down a hill, the camera follows the slope, for example.
    // Default value is derived from trial and error!
    const float kDefaultHeight = 60.0f;
    float mHeight = kDefaultHeight;

    // If true, the camera code doesn't care about floor height and pretty much just ignores it entirely.
    bool mIgnoreFloor = false;

    //////////////////
    // COLLISION
    //////////////////
    // A model whose triangles are used as collision for the camera.
    std::vector<Model*> mBoundsModels;

    // If true, camera bounds are turned on. If false, they are disabled.
    bool mBoundsEnabled = true;

    //////////////////
    // GLIDING
    //////////////////
    // If true, we are performing a glide operation/
    bool mGliding = false;

    // Start glide position/rotation.
    Vector3 mGlideStartPos;
    Quaternion mGlideStartRot;

    // End glide position/rotation.
    Vector3 mGlidePosition;
    Quaternion mGlideRotation;

    // Duration of the glide.
    // The original game uses an EXTREMELY short duration - imo it looks a bit better slower.
    const float kGlideDuration = 0.25f;
    float mGlideTimer = 0.0f;

    // Callback for end of glide.
    std::function<void()> mEndGlideCallback = nullptr;

    //////////////////
    // INSPECTION
    //////////////////
    // The scene noun we are inspecting.
    std::string mInspectNoun;

    // Start position/rotation when inspecting (so we can later do an uninspect).
    Vector3 mInspectStartPos;
    Vector3 mInspectStartYawPitch;

    void SceneUpdate(float deltaTime);
    void SceneUpdateMovement(float deltaTime);
    void SceneUpdateInteract(float deltaTime);

    GKObject* RaycastIntoScene(const Ray& ray, bool interactiveOnly);

    Vector3 ResolveCollisions(const Vector3& startPosition, const Vector3& moveOffset);
};
