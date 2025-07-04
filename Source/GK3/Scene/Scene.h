//
// Clark Kromenaker
//
// Encapsulates all the loaded actors and components for a single conceptual
// location or place in the game. A scene can represent a single level, a set
// of menus, or a particular location in a game.
//
// For GK3, a "scene" is a particular location in the game world (Lobby, Dining Room,
// Room 25, Train Station, etc). The game can only be "in" one scene at a time.
//
// From GK3 docs: "a stage is everything associated with a specific game location
// during a specific timeblock (e.g. Dining Room, Day 1, 10am)."
//
#pragma once
#include <string>
#include <vector>

#include "Collisions.h"
#include "SceneConstruction.h"
#include "SceneData.h"
#include "SceneLayer.h"
#include "Timeblock.h"

class ActionBar;
class Animator;
class BSP;
class BSPActor;
class GameCamera;
class GKActor;
class GKObject;
class GKProp;
class Ray;
struct SceneModel;
class SIF;
class Skybox;
class SoundtrackPlayer;
class StatusOverlay;
class Vector3;

struct SceneCastResult
{
    RaycastHit hitInfo;
    GKObject* hitObject = nullptr;
};

class Scene
{
public:
    static const char* GetEgoName();

    static Animator* GetGlobalAnimator();
    static Animator* GetActiveAnimator();

    Scene(const std::string& name);
    ~Scene();

    void Load();
    void Unload();
    bool IsLoaded() const { return mSceneData != nullptr; }

    void Init();
    void Update(float deltaTime);

    bool InitEgoPosition(const std::string& positionName);
    void SetCameraPosition(const std::string& cameraName);
    void SetCameraPositionForConversation(const std::string& conversationName, bool isInitial);
    void GlideToCameraPosition(const std::string& cameraName, std::function<void()> finishCallback);

    SceneCastResult Raycast(const Ray& ray, bool interactiveOnly, GKObject** ignore = nullptr, int ignoreCount = 1) const;
    SceneCastResult RaycastAABBs(const Ray& ray, GKObject** ignore = nullptr, int ignoreCount = 1) const;
    void Interact(const Ray& ray, GKObject* interactHint = nullptr);
    void SkipCurrentAction();

    GKActor* GetEgo() const { return mEgo; }
    void SetEgo(const std::string& actorName);

    BSPActor* GetHitTestObjectByModelName(const std::string& modelName) const;
    GKObject* GetSceneObjectByModelName(const std::string& modelName) const;
    GKProp* GetPropByModelName(const std::string& modelName) const;

    GKObject* GetSceneObjectByNoun(const std::string& noun) const;
    GKProp* GetPropByNoun(const std::string& noun) const;
    GKActor* GetActorByNoun(const std::string& noun) const;

    const ScenePosition* GetPosition(const std::string& positionName) const;
    float GetFloorY(const Vector3& position) const;
    bool GetFloorInfo(const Vector3& position, float& outHeight, Texture*& outTexture);

    void ApplyTextureToSceneModel(const std::string& modelName, Texture* texture);
    void SetSceneModelVisibility(const std::string& modelName, bool visible);
    bool IsSceneModelVisible(const std::string& modelName) const;
    bool DoesSceneModelExist(const std::string& modelName) const;

    void SetFixedModelLighting(const std::string& modelName, const Color32& color);

    void SetPaused(bool paused);

    void InspectActiveObject(const std::function<void()>& finishCallback);
    void InspectObject(const std::string& noun, const std::function<void()>& finishCallback);
    void InspectObject(const std::string& noun, const std::string& cameraName, const std::function<void()>& finishCallback);
    void InspectObject(const std::string& noun, const SceneCamera* camera, const std::function<void()>& finishCallback);
    void UninspectObject(const std::function<void()>& finishCallback);

    void SetOverrideBSP(BSP* bsp);
    void ClearOverrideBSP();

    void SetWalkOverride(const std::function<void()>& callback) { mWalkOverrideCallback = callback; }

    SceneData* GetSceneData() const { return mSceneData; }
    Animator* GetAnimator() const { return mAnimator; }
    SoundtrackPlayer* GetSoundtrackPlayer() const { return mSoundtrackPlayer; }
    GameCamera* GetCamera() const { return mCamera; }
    StatusOverlay* GetStatusOverlay() const { return mStatusOverlay; }
    SceneConstruction& GetConstruction() { return mConstruction; }

private:
    // Location is 3-letter code (e.g. DIN).
    std::string mLocation;

    // Timeblock is day/time code (e.g. 110A).
    Timeblock mTimeblock;

    // Layer for the scene.
    SceneLayer mLayer;

    // Contains scene data references for the current location/timeblock.
    // If not null, means we're loaded!
    SceneData* mSceneData = nullptr;

    // On rare occasions (binocs), the scene's BSP gets overridden with another scene's BSP.
    BSP* mOverrideBSP = nullptr;

    // The animation player for the scene.
    Animator* mAnimator = nullptr;

    // The soundtrack player for the scene.
    SoundtrackPlayer* mSoundtrackPlayer = nullptr;

    // The game camera used to move around.
    GameCamera* mCamera = nullptr;

    // The status overlay for the scene.
    StatusOverlay* mStatusOverlay = nullptr;

    // All actors and props in one list.
    // Sometimes, we want to treat these guys in a homogenous manner, since both have Models, can be interacted with, etc.
    std::vector<GKObject*> mPropsAndActors;

    // Just "actors".
    std::vector<GKActor*> mActors;

    // Just "props".
    std::vector<GKProp*> mProps;

    // Actors in the BSP.
    std::vector<BSPActor*> mBSPActors;

    // Actors that are marked as hit test models.
    std::vector<BSPActor*> mHitTestActors;

    // The skyboxes can also have hit tests, but they use a different system!
    // In this case, a palette index is mapped to a simple object with a noun.
    std::unordered_map<uint8_t, GKObject*> mSkyboxHitTests;

    // The Actor the player is controlling in this scene.
    const SceneActor* mEgoSceneActor = nullptr;
    GKActor* mEgo = nullptr;

    // The name of the last Ego the player was controlling, including the current scene.
    // This is static so we can query who was the last Ego even if a scene is not loaded (e.g. on the Map).
    static std::string mEgoName;

    // The most recently "active" object.
    // In other words, the last object the action bar was shown for.
    GKObject* mActiveObject = nullptr;

    // If true, the scene is paused (won't update).
    bool mPaused = false;

    // Helper class for dealing with scene construction (e.g. editor/tool) support.
    SceneConstruction mConstruction;

    // If set, Ego won't walk when the floor is clicked - instead this callback will be called.
    std::function<void()> mWalkOverrideCallback = nullptr;

    void ApplyAmbientLightColorToActors();
    BSP* GetBSP() const;
    void ExecuteAction(const Action* action);
};

/*
What do we need to do to load a stage?
 1) Parse general SIF asset for the stage.
 2) Parse specific SIF asset for the current timeblock.
 3) Parse SCN asset specified in SIF.
 4) Parse BSP asset specified in SCN.
 5) Set BSP to be rendered.
 6) Load and use boundary map for walking
 7) Load and use camera bounds for the camera movement.
 8) Set global light values.
 9) Load and render skybox.
 10) Load, place, and initialize any actors in the stage.
 11) Load, place, and initialize any models in the stage.
 12) Position camera based on "default" ROOM_CAMERAS entry (or first item).
 13) Load and parse NVC files for current day.
*/
