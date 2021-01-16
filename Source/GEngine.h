//
// GEngine.h
//
// Clark Kromenaker
//
// The "main" engine class. This is created by main() and handles
// init, running the game loop, shutdown, and some coordination between systems.
//
#pragma once
#include <vector>

#include "ActionManager.h"
#include "AssetManager.h"
#include "AudioManager.h"
#include "Console.h"
#include "InputManager.h"
#include "LayerManager.h"
#include "Renderer.h"
#include "SheepManager.h"
#include "ReportManager.h"
#include "VideoPlayer.h"

class Actor;
class Scene;
class Cursor;

class GEngine
{
public:
    static GEngine* Instance() { return sInstance; }
    
    GEngine();
    
    bool Initialize();
    void Shutdown();
    void Run();
    
    void Quit();
    
    void AddActor(Actor* actor);
    
	void LoadScene(std::string name) { mSceneToLoad = name; }
    Scene* GetScene() { return mScene; }
	
	void UseDefaultCursor();
	void UseHighlightCursor();
	void UseWaitCursor();
    
private:
    // Only one instance of GEngine can exist.
    static GEngine* sInstance;
    
    // Is the game running? While true, we loop. When false, the game exits.
	// False by default, but set to true after initialization.
	bool mRunning = false;
    
    // Subsystems.
    Renderer mRenderer;
    AudioManager mAudioManager;
    AssetManager mAssetManager;
    InputManager mInputManager;
    SheepManager mSheepManager;
	ReportManager mReportManager;
	ActionManager mActionManager;
	Console mConsole;
    VideoPlayer mVideoPlayer;
    LayerManager mLayerManager;
    
    // A list of all actors that currently exist in the game.
    std::vector<Actor*> mActors;
    
    // The currently active scene. There can be only one at a time (sure about that?).
    Scene* mScene = nullptr;
	
	// A scene that's been requested to load. If empty, no pending scene change.
	// Scene loads happen at the end of a frame, to avoid a scene change mid-frame.
	std::string mSceneToLoad;
	
	// Cursors - maybe move these into a manager at some point?
	Cursor* mActiveCursor = nullptr;
	Cursor* mDefaultCursor = nullptr;
	Cursor* mHighlightRedCursor = nullptr;
	Cursor* mHighlightBlueCursor = nullptr;
	Cursor* mWaitCursor = nullptr;
	
	// The currently active cursor.
    Cursor* mCursor = nullptr;
    
    void ProcessInput();
    void Update();
    void GenerateOutputs();
	
	void LoadSceneInternal();
	
	void DeleteDestroyedActors();
};
