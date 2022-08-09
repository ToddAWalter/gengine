#include "SheepAPI_Actors.h"

#include "FaceController.h"
#include "GEngine.h"
#include "GKActor.h"
#include "Scene.h"

using namespace std;

shpvoid Blink(const std::string& actorName)
{
    GKActor* actor = GEngine::Instance()->GetScene()->GetActorByNoun(actorName);
    if(actor != nullptr)
    {
        actor->GetFaceController()->Blink();
    }
    else
    {
        ExecError();
    }
    return 0;
}
RegFunc1(Blink, void, string, IMMEDIATE, REL_FUNC);

shpvoid BlinkX(const std::string& actorName, const std::string& blinkAnim)
{
    GKActor* actor = GEngine::Instance()->GetScene()->GetActorByNoun(actorName);
    if(actor != nullptr)
    {
        actor->GetFaceController()->Blink(blinkAnim);
    }
    else
    {
        ExecError();
    }
    return 0;
}
RegFunc2(BlinkX, void, string, string, IMMEDIATE, REL_FUNC);

shpvoid EnableEyeJitter(const std::string& actorName)
{
    GKActor* actor = GEngine::Instance()->GetScene()->GetActorByNoun(actorName);
    if(actor != nullptr)
    {
        actor->GetFaceController()->SetEyeJitterEnabled(true);
    }
    else
    {
        ExecError();
    }
    return 0;
}
RegFunc1(EnableEyeJitter, void, string, IMMEDIATE, REL_FUNC);

shpvoid DisableEyeJitter(const std::string& actorName)
{
    GKActor* actor = GEngine::Instance()->GetScene()->GetActorByNoun(actorName);
    if(actor != nullptr)
    {
        actor->GetFaceController()->SetEyeJitterEnabled(false);
    }
    else
    {
        ExecError();
    }
    return 0;
}
RegFunc1(DisableEyeJitter, void, string, IMMEDIATE, REL_FUNC);

shpvoid EyeJitter(const std::string& actorName)
{
    GKActor* actor = GEngine::Instance()->GetScene()->GetActorByNoun(actorName);
    if(actor != nullptr)
    {
        actor->GetFaceController()->EyeJitter();
    }
    else
    {
        ExecError();
    }
    return 0;
}
RegFunc1(EyeJitter, void, string, IMMEDIATE, REL_FUNC);

/*
shpvoid Glance(std::string actorName, int percentX, int percentY, int durationMs)
{
    return 0;
}

shpvoid GlanceX(std::string actorName, int leftPercentX, int leftPercentY,
                int rightPercentX, int rightPercentY, int durationMs)
{
    return 0;
}
*/

shpvoid SetMood(const std::string& actorName, const std::string& moodName)
{
    // Get actor and make sure it's valid.
    GKActor* actor = GEngine::Instance()->GetScene()->GetActorByNoun(actorName);
    if(actor == nullptr)
    {
        ExecError();
        return 0;
    }

    // Apply mood.
    //TODO: What if mood is invalid?
    actor->GetFaceController()->SetMood(moodName);
    return 0;
}
RegFunc2(SetMood, void, string, string, IMMEDIATE, REL_FUNC);

shpvoid ClearMood(const std::string& actorName)
{
    // Get actor and make sure it's valid.
    GKActor* actor = GEngine::Instance()->GetScene()->GetActorByNoun(actorName);
    if(actor == nullptr)
    {
        ExecError();
        return 0;
    }

    // Clear mood.
    actor->GetFaceController()->ClearMood();
    return 0;
}
RegFunc1(ClearMood, void, string, IMMEDIATE, REL_FUNC);

shpvoid Expression(const std::string& actorName, const std::string& expression)
{
    GKActor* actor = GEngine::Instance()->GetScene()->GetActorByNoun(actorName);
    if(actor != nullptr)
    {
        actor->GetFaceController()->DoExpression(expression);
    }
    else
    {
        ExecError();
    }
    return 0;
}
RegFunc2(Expression, void, string, string, IMMEDIATE, REL_FUNC);

shpvoid SetIdleGAS(const std::string& actorName, const std::string& gasName)
{
    // Get actor and make sure it's valid.
    GKActor* actor = GEngine::Instance()->GetScene()->GetActorByNoun(actorName);
    if(actor == nullptr)
    {
        ExecError();
        return 0;
    }

    // If gas is empty, that means clear out fidget (not an error).
    if(gasName.empty())
    {
        actor->SetIdleFidget(nullptr);
        return 0;
    }

    // Load the fidget.
    // If the fidget doesn't exist, we still set it, but we output an error.
    GAS* fidget = Services::GetAssets()->LoadGAS(gasName);
    if(fidget == nullptr)
    {
        Services::GetReports()->Log("Error", "Attempted to load an invalid fidget file: " + gasName);
    }
    actor->SetIdleFidget(fidget);
    return 0;
}
RegFunc2(SetIdleGAS, void, string, string, WAITABLE, REL_FUNC); // NOTE: function is waitable, but does nothing.

shpvoid SetListenGAS(const std::string& actorName, const std::string& gasName)
{
    // Get actor and make sure it's valid.
    GKActor* actor = GEngine::Instance()->GetScene()->GetActorByNoun(actorName);
    if(actor == nullptr)
    {
        ExecError();
        return 0;
    }

    // If gas is empty, that means clear out fidget (not an error).
    if(gasName.empty())
    {
        actor->SetListenFidget(nullptr);
        return 0;
    }

    // Load the fidget.
    // If the fidget doesn't exist, we still set it, but we output an error.
    GAS* fidget = Services::GetAssets()->LoadGAS(gasName);
    if(fidget == nullptr)
    {
        Services::GetReports()->Log("Error", "Attempted to load an invalid fidget file: " + gasName);
    }
    actor->SetListenFidget(fidget);
    return 0;
}
RegFunc2(SetListenGAS, void, string, string, WAITABLE, REL_FUNC); // NOTE: function is waitable, but does nothing.

shpvoid SetTalkGAS(const std::string& actorName, const std::string& gasName)
{
    // Get actor and make sure it's valid.
    GKActor* actor = GEngine::Instance()->GetScene()->GetActorByNoun(actorName);
    if(actor == nullptr)
    {
        ExecError();
        return 0;
    }

    // If gas is empty, that means clear out fidget (not an error).
    if(gasName.empty())
    {
        actor->SetTalkFidget(nullptr);
        return 0;
    }

    // Load the fidget.
    // If the fidget doesn't exist, we still set it, but we output an error.
    GAS* fidget = Services::GetAssets()->LoadGAS(gasName);
    if(fidget == nullptr)
    {
        Services::GetReports()->Log("Error", "Attempted to load an invalid fidget file: " + gasName);
    }
    actor->SetTalkFidget(fidget);
    return 0;
}
RegFunc2(SetTalkGAS, void, string, string, WAITABLE, REL_FUNC); // NOTE: function is waitable, but does nothing.

shpvoid StartIdleFidget(const std::string& actorName)
{
    GKActor* actor = GEngine::Instance()->GetScene()->GetActorByNoun(actorName);
    if(actor == nullptr)
    {
        ExecError();
        return 0;
    }

    actor->StartFidget(GKActor::FidgetType::Idle);
    return 0;
}
RegFunc1(StartIdleFidget, void, string, WAITABLE, REL_FUNC);

shpvoid StartListenFidget(const std::string& actorName)
{
    GKActor* actor = GEngine::Instance()->GetScene()->GetActorByNoun(actorName);
    if(actor == nullptr)
    {
        ExecError();
        return 0;
    }

    actor->StartFidget(GKActor::FidgetType::Listen);
    return 0;
}
RegFunc1(StartListenFidget, void, string, WAITABLE, REL_FUNC);

shpvoid StartTalkFidget(const std::string& actorName)
{
    GKActor* actor = GEngine::Instance()->GetScene()->GetActorByNoun(actorName);
    if(actor == nullptr)
    {
        ExecError();
        return 0;
    }

    actor->StartFidget(GKActor::FidgetType::Talk);
    return 0;
}
RegFunc1(StartTalkFidget, void, string, WAITABLE, REL_FUNC);

shpvoid StopFidget(const std::string& actorName)
{
    GKActor* actor = GEngine::Instance()->GetScene()->GetActorByNoun(actorName);
    if(actor == nullptr)
    {
        ExecError();
    }
    else
    {
        actor->StopFidget(AddWait());
    }
    return 0;
}
RegFunc1(StopFidget, void, string, WAITABLE, REL_FUNC);

/*
 shpvoid SetWalkAnim(std::string actorName, std::string start, std::string cont,
                     std::string startTurnLeft, std::string startTurnRight)
 {
     std::cout << "SetWalkAnim" << std::endl;
     return 0;
 }
 RegFunc5(SetWalkAnim, void, string, string, string, string, string, IMMEDIATE, REL_FUNC);
 
shpvoid TurnHead(std::string actorName, int percentX, int percentY, int durationMs)
{
 std::cout << "TurnHead" << std::endl;
 return 0;
}
RegFunc4(TurnHead, void, string, int, int, int, WAITABLE, REL_FUNC);

shpvoid TurnToModel(std::string actorName, std::string modelName)
{
 std::cout << "TurnToModel" << std::endl;
 return 0;
}
RegFunc2(TurnToModel, void, string, string, WAITABLE, REL_FUNC);

shpvoid LookitActor(std::string actorName, std::string otherActorName,
                     std::string componentsSpec, float durationSec)
{
    return 0;
}
RegFunc4(LookitActor, void, string, string, string, float, IMMEDIATE, REL_FUNC);

shpvoid LookitActorQuick(std::string actorName, std::string otherActorName,
                         std::string componentsSpec, float durationSec)
{
    return 0;
}
RegFunc4(LookitActorQuick, void, string, string, string, float, IMMEDIATE, REL_FUNC);

shpvoid LookitModel(std::string actorName, std::string modelName,
                    std::string componentsSpec, float durationSec)
{
    return 0;
}
RegFunc4(LookitModel, void, string, string, string, float, IMMEDIATE, REL_FUNC);

shpvoid LookitModelQuick(std::string actorName, std::string modelName,
                         std::string componentsSpec, float durationSec)
{
    return 0;
}
RegFunc4(LookitModelQuick, void, string, string, string, float, IMMEDIATE, REL_FUNC);

shpvoid LookitModelX(std::string actorName, std::string modelName, int mesh,
                     std::string boxModifier, float offsetX, float offsetY, float offsetZ,
                     std::string componentsSpec, float durationSec)
{
    return 0;
}
//TODO: Need a new RegFuncX for 9 arguments!

//LookitModelQuickX

//LookitNoun

//LookitNounQuick

//LookitPoint

//LookitCameraAngle

//LookitMouse

//LookitPlayer

//LookitCancel
*/