#include "SoundtrackPlayer.h"

#include "ActionManager.h"
#include "Actor.h"
#include "GKObject.h"
#include "StringUtil.h"

PlayingSoundtrack::PlayingSoundtrack(Soundtrack* soundtrack) :
    mSoundtrack(soundtrack)
{

}

void PlayingSoundtrack::Play()
{
    // Make sure all nodes are reset.
    mExecutionCounts.clear();
    mExecutionCounts.resize(mSoundtrack->GetNodes().size());

    // Get things rolling.
    ProcessNextNode();
}

void PlayingSoundtrack::Stop(bool force)
{
    // If a sound is currently playing as part of this soundtrack, we need to stop it in the desired fashion.
    if(mSoundtrackNodeResults.stopMethod == StopMethod::Immediate)
    {
        mSoundtrackNodeResults.soundHandle.Stop();
    }
    else if(mSoundtrackNodeResults.stopMethod == StopMethod::FadeOut)
    {
        mSoundtrackNodeResults.soundHandle.Stop(static_cast<float>(mSoundtrackNodeResults.fadeOutTimeMs) * 0.001f);
    }
    else if(mSoundtrackNodeResults.stopMethod == StopMethod::PlayToEnd && force)
    {
        // In some cases, a soundtrack is forced to stop, such as when moving to a new scene.
        // In these cases, even "play to end" sounds will stop.
        mSoundtrackNodeResults.soundHandle.Stop();
    }
}

void PlayingSoundtrack::Update(float deltaTime)
{
    // If the previous node designated a follow object, keep the playing sound's 3D position updated to "follow" that object.
    if(mSoundtrackNodeResults.followObj != nullptr)
    {
        mSoundtrackNodeResults.soundHandle.SetPosition(mSoundtrackNodeResults.followObj->GetAudioPosition());
    }

    // If the current node is looping, we can early out - there is NO way to advance past a looping node.
    // The actual looping behavior is controlled by the audio system.
    if(mCurrentNodeIndex >= 0 && mSoundtrack->GetNodes()[mCurrentNodeIndex]->IsLooping())
    {
        return;
    }

    // Decrement timer. When it gets to zero, we move onto the next node.
    if(mTimer >= 0.0f)
    {
        mTimer -= deltaTime;
        if(mTimer <= 0.0f)
        {
            ProcessNextNode();
        }
    }
}

void PlayingSoundtrack::ProcessNextNode()
{
    // If no nodes, can't do anything!
    auto& nodes = mSoundtrack->GetNodes();
    if(nodes.empty())
    {
        mTimer = 0.0f;
        return;
    }

    // Update the node index and loop if necessary.
    mCurrentNodeIndex++;
    mCurrentNodeIndex %= nodes.size();

    // Grab the node from the list.
    SoundtrackNode* node = nodes[mCurrentNodeIndex];

    // If this node has a repeat limit, check whether it has been surpassed.
    // If so, just move on to the next node right away.
    if(node->repeat > 0 && node->repeat - mExecutionCounts[mCurrentNodeIndex] <= 0)
    {
        mTimer = 0.0f;
        return;
    }

    // Ok, execute the thing.
    mExecutionCounts[mCurrentNodeIndex]++;
    int waitMilliseconds = node->Execute(mSoundtrack, mSoundtrackNodeResults);
    mTimer = static_cast<float>(waitMilliseconds) / 1000.0f;
}

TYPEINFO_INIT(SoundtrackPlayer, Component, 5)
{

}

SoundtrackPlayer::SoundtrackPlayer(Actor* owner) : Component(owner)
{

}

SoundtrackPlayer::~SoundtrackPlayer()
{
    // When a soundtrack player is destructed, all its playing soundtracks should stop.
    // This causes audio to either end immediately or fade out.
    //
    // For sounds with a stop mode of "PlayToEnd", they WILL actually continue to play - that's expected!
    // Any "PlayToEnd" sound that should be stopped when the player is destroyed should have Stop explicitly called with the force flag set.
    StopAll();
}

void SoundtrackPlayer::Play(Soundtrack* soundtrack)
{
    if(soundtrack == nullptr) { return; }

    // See if already playing. If so, don't do anything!
    for(auto& playing : mPlaying)
    {
        if(playing.mSoundtrack == soundtrack)
        {
            return;
        }
    }

    // Ok, we are going to play this thing.
    mPlaying.emplace_back(soundtrack);
    mPlaying.back().Play();
}

void SoundtrackPlayer::Stop(Soundtrack* soundtrack, bool force)
{
    if(soundtrack == nullptr) { return; }

    // Find and remove.
    for(auto it = mPlaying.begin(); it != mPlaying.end(); ++it)
    {
        if(it->mSoundtrack == soundtrack)
        {
            // Stop the soundtrack.
            // How the audio stops depends on the "Stop Method" of the current soundtrack node.
            it->Stop(force);

            // Erase from list.
            mPlaying.erase(it);
            return;
        }
    }
}

void SoundtrackPlayer::Stop(const std::string& soundtrackName)
{
    // Find and remove.
    for(auto it = mPlaying.begin(); it != mPlaying.end(); ++it)
    {
        if(StringUtil::EqualsIgnoreCase(it->mSoundtrack->GetNameNoExtension(), soundtrackName))
        {
            // Stop the soundtrack.
            // How the audio stops depends on the "Stop Method" of the current soundtrack node.
            it->Stop();

            // Erase from list.
            mPlaying.erase(it);
            return;
        }
    }
}

void SoundtrackPlayer::StopAll(bool force)
{
    // Stop all playing soundtracks.
    for(PlayingSoundtrack& playing : mPlaying)
    {
        playing.Stop(force);
    }
    mPlaying.clear();
}

void SoundtrackPlayer::OnUpdate(float deltaTime)
{
    if(gActionManager.IsSkippingCurrentAction()) { return; }

    for(PlayingSoundtrack& playing : mPlaying)
    {
        playing.Update(deltaTime);
    }
}
