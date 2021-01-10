//
// Soundtrack.h
//
// Clark Kromenaker
//
// Represents a "Soundtrack" asset, used to create dynamic musical
// scores for game locales using music snippets and node-based logic.
//
// In-memory representation of ".STK" files.
//
#pragma once
#include "Asset.h"

#include <string>
#include <vector>
#include <cstdlib>

#include "Vector3.h"

struct IniSection;

enum class SoundtrackSoundType
{
    Ambient,
    SFX
};

struct SoundtrackNode
{
    virtual int Execute(SoundtrackSoundType soundType) = 0;
    
    virtual bool IsLooping()
    {
        return false;
    }
    
    virtual void Reset()
    {
        executionCount = 0;
    }
    
protected:
    int executionCount = 0;
};

struct WaitNode : public SoundtrackNode
{
    int minWaitTimeMs = 0;
    int maxWaitTimeMs = 0;
    int repeat = 0;
    int random = 100;
    
    int Execute(SoundtrackSoundType soundType) override;
};

struct SoundNode : public SoundtrackNode
{
    std::string soundName;
    int volume = 100;
    int repeat = 0;
    int random = 100;
    bool loop = false;
    int fadeInTimeMs = 0;
    int fadeOutTimeMs = 0;
    int stopMethod = 0;
    bool is3d = false;
    float minDist = 0.0f;
    float maxDist = 0.0f;
    Vector3 position;
    std::string followModelName;
    
    bool IsLooping() override { return loop; }
    int Execute(SoundtrackSoundType soundType) override;
};

struct PrsNode : public SoundtrackNode
{
    std::vector<SoundNode*> soundNodes;
    
    int Execute(SoundtrackSoundType soundType) override
    {
        if(soundNodes.size() == 0) { return 0; }
        
        int randomIndex = rand() % soundNodes.size();
        return soundNodes[randomIndex]->Execute(soundType);
    }
};

class Soundtrack : public Asset
{
public:
    Soundtrack(std::string name, char* data, int dataLength);
    
    SoundtrackSoundType GetSoundType() const { return mSoundType; }
    std::vector<SoundtrackNode*> GetNodesCopy() const { return mNodes; }
    
private:
    // Type indicates whether this audio is considered music or SFX or what.
    SoundtrackSoundType mSoundType = SoundtrackSoundType::Ambient;
    
    // A soundtrack is a list of nodes that play audio or wait X seconds.
    std::vector<SoundtrackNode*> mNodes;
    
    void ParseFromData(char* data, int dataLength);
    SoundNode* ParseSoundNodeFromSection(IniSection& section);
};
