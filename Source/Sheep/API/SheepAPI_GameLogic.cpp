#include "SheepAPI_GameLogic.h"

#include "ActionBar.h"
#include "ActionManager.h"
#include "GameProgress.h"
#include "GameTimers.h"
#include "GEngine.h"
#include "Random.h"
#include "ReportManager.h"
#include "SceneManager.h"
#include "SheepManager.h"
#include "Timers.h"
#include "VerbManager.h"

using namespace std;

int GetScore()
{
    return gGameProgress.GetScore();
}
RegFunc0(GetScore, int, IMMEDIATE, REL_FUNC);

shpvoid IncreaseScore(int value)
{
    gGameProgress.IncreaseScore(value);
    return 0;
}
RegFunc1(IncreaseScore, void, int, IMMEDIATE, REL_FUNC);

shpvoid SetScore(int score)
{
    gGameProgress.SetScore(score);
    return 0;
}
RegFunc1(SetScore, void, int, IMMEDIATE, DEV_FUNC);

shpvoid ChangeScore(const std::string& scoreValue)
{
    gGameProgress.ChangeScore(scoreValue);
    return 0;
}
RegFunc1(ChangeScore, void, string, IMMEDIATE, REL_FUNC);

int GetFlag(const std::string& flagName)
{
    return gGameProgress.GetFlag(flagName);
}
RegFunc1(GetFlag, int, string, IMMEDIATE, REL_FUNC);

/*
int GetFlagInt(int flagEnum)
{
    // This function gets a flag, not by name, but by internal integer value.
    // I'm a bit unclear how any Sheep caller would know this value, and how
    // this value would be persistant across runs of the game...
    std::cout << "GetFlagInt was called!" << std::endl;
    return 0;
}
RegFunc1(GetFlagInt, int, int, IMMEDIATE, REL_FUNC);
*/

shpvoid SetFlag(const std::string& flagName)
{
    gGameProgress.SetFlag(flagName);
    return 0;
}
RegFunc1(SetFlag, void, string, IMMEDIATE, REL_FUNC);

shpvoid ClearFlag(const std::string& flagName)
{
    gGameProgress.ClearFlag(flagName);
    return 0;
}
RegFunc1(ClearFlag, void, string, IMMEDIATE, REL_FUNC);

shpvoid DumpFlags()
{
    gGameProgress.DumpFlags();
    return 0;
}
RegFunc0(DumpFlags, void, IMMEDIATE, DEV_FUNC);

int GetGameVariableInt(const std::string& varName)
{
    return gGameProgress.GetGameVariable(varName);
}
RegFunc1(GetGameVariableInt, int, string, IMMEDIATE, REL_FUNC);

shpvoid IncGameVariableInt(const std::string& varName)
{
    gGameProgress.IncGameVariable(varName);
    return 0;
}
RegFunc1(IncGameVariableInt, void, string, IMMEDIATE, REL_FUNC);

shpvoid SetGameVariableInt(const std::string& varName, int value)
{
    gGameProgress.SetGameVariable(varName, value);
    return 0;
}
RegFunc2(SetGameVariableInt, void, string, int, IMMEDIATE, REL_FUNC);

int GetNounVerbCount(const std::string& noun, const std::string& verb)
{
    return gGameProgress.GetNounVerbCount(noun, verb);
}
RegFunc2(GetNounVerbCount, int, string, string, IMMEDIATE, REL_FUNC);

int GetNounVerbCountInt(int nounEnum, int verbEnum)
{
    return GetNounVerbCount(gActionManager.GetNoun(nounEnum),
                            gActionManager.GetVerb(verbEnum));
}
RegFunc2(GetNounVerbCountInt, int, int, int, IMMEDIATE, REL_FUNC);

shpvoid IncNounVerbCount(const string& noun, const string& verb)
{
    //TODO: Throw an error if the given noun corresponds to a "Topic".
    gGameProgress.IncNounVerbCount(noun, verb);
    return 0;
}
RegFunc2(IncNounVerbCount, void, string, string, IMMEDIATE, REL_FUNC);

shpvoid IncNounVerbCountBoth(const string& noun, const string& verb)
{
    //TODO: Throw an error if the given noun corresponds to a "Topic".
    gGameProgress.IncNounVerbCount("Gabriel", noun, verb);
    gGameProgress.IncNounVerbCount("Grace", noun, verb);
    return 0;
}
RegFunc2(IncNounVerbCountBoth, void, string, string, IMMEDIATE, REL_FUNC);

shpvoid SetNounVerbCount(const std::string& noun, const std::string& verb, int count)
{
    //TODO: Throw an error if the given noun corresponds to a "Topic".
    gGameProgress.SetNounVerbCount(noun, verb, count);
    return 0;
}
RegFunc3(SetNounVerbCount, void, string, string, int, IMMEDIATE, REL_FUNC);

shpvoid SetNounVerbCountBoth(const std::string& noun, const std::string& verb, int count)
{
    //TODO: Throw an error if the given noun corresponds to a "Topic".
    gGameProgress.SetNounVerbCount("Gabriel", noun, verb, count);
    gGameProgress.SetNounVerbCount("Grace", noun, verb, count);
    return 0;
}
RegFunc3(SetNounVerbCountBoth, void, string, string, int, IMMEDIATE, REL_FUNC);

shpvoid TriggerNounVerb(const std::string& noun, const std::string& verb)
{
    //TODO: Validate noun or throw error.
    //TODO: Validate verb or throw error.
    bool success = gActionManager.ExecuteAction(noun, verb);
    if(!success)
    {
        gReportManager.Log("Error", "Error: unable to trigger noun-verb combination " + noun + ":" + verb);
    }
    return 0;
}
RegFunc2(TriggerNounVerb, void, string, string, IMMEDIATE, DEV_FUNC);

int GetTopicCount(const std::string& noun, const std::string& verb)
{
    //TODO: Validate noun. Must be a valid noun. Seems to include any scene nouns, inventory nouns, actor nouns.
    if(!gVerbManager.IsTopic(verb))
    {
        gReportManager.Log("Error", "Error: '" + verb + " is not a valid verb name.");
        return 0;
    }
    return gGameProgress.GetTopicCount(noun, verb);
}
RegFunc2(GetTopicCount, int, string, string, IMMEDIATE, REL_FUNC);

int GetTopicCountInt(int nounEnum, int verbEnum)
{
    std::string noun = gActionManager.GetNoun(nounEnum);
    std::string verb = gActionManager.GetVerb(verbEnum);
    return GetTopicCount(noun, verb);
}
RegFunc2(GetTopicCountInt, int, int, int, IMMEDIATE, REL_FUNC);

int HasTopicsLeft(const std::string& noun)
{
    //TODO: Validate noun.
    bool hasTopics = gActionManager.HasTopicsLeft(noun);
    return hasTopics ? 1 : 0;
}
RegFunc1(HasTopicsLeft, int, string, IMMEDIATE, REL_FUNC);

shpvoid SetTopicCount(std::string noun, std::string verb, int count)
{
    //TODO: Validate noun or report error.
    //TODO: Validate verb or report error.
    gGameProgress.SetTopicCount(noun, verb, count);
    return 0;
}
RegFunc3(SetTopicCount, void, string, string, int, IMMEDIATE, DEV_FUNC);

int GetChatCount(const std::string& noun)
{
    return gGameProgress.GetChatCount(noun);
}
RegFunc1(GetChatCount, int, string, IMMEDIATE, REL_FUNC);

int GetChatCountInt(int nounEnum)
{
    return GetChatCount(gActionManager.GetNoun(nounEnum));
}
RegFunc1(GetChatCountInt, int, int, IMMEDIATE, REL_FUNC);

shpvoid SetChatCount(std::string noun, int count)
{
    gGameProgress.SetChatCount(noun, count);
    return 0;
}
RegFunc2(SetChatCount, void, string, int, IMMEDIATE, DEV_FUNC);

shpvoid SetVerbModal(int modalState)
{
    gActionManager.GetActionBar()->SetAllowDismiss(modalState == 0);
    return 0;
}
RegFunc1(SetVerbModal, void, int, IMMEDIATE, REL_FUNC);

shpvoid StartVerbCancel()
{
    gActionManager.GetActionBar()->SetAllowCancel(true);
    return 0;
}
RegFunc0(StartVerbCancel, void, IMMEDIATE, REL_FUNC);

shpvoid StopVerbCancel()
{
    gActionManager.GetActionBar()->SetAllowCancel(false);
    return 0;
}
RegFunc0(StopVerbCancel, void, IMMEDIATE, REL_FUNC);

shpvoid SetGameTimer(const std::string& noun, const std::string& verb, int milliseconds)
{
    GameTimers::Set(noun, verb, milliseconds * 0.001f);
    return 0;
}
RegFunc3(SetGameTimer, void, string, string, int, IMMEDIATE, REL_FUNC);

shpvoid SetTimerMs(int milliseconds)
{
    // Should throw error if not waited upon!
    SheepThread* currentThread = gSheepManager.GetCurrentThread();
    if(!currentThread->mInWaitBlock)
    {
        gReportManager.Log("Warning", "No point setting a timer if you don't wait for it to finish. " +
                                                std::to_string(milliseconds) + " millisecond timer request ignored.");
        ExecError();
    }
    else
    {
        Timers::AddTimerMilliseconds(static_cast<unsigned int>(milliseconds), currentThread->AddWait());
    }
    return 0;
}
RegFunc1(SetTimerMs, void, int, WAITABLE, REL_FUNC);

shpvoid SetTimerSeconds(float seconds)
{
    // Should throw error if not waited upon!
    SheepThread* currentThread = gSheepManager.GetCurrentThread();
    if(!currentThread->mInWaitBlock)
    {
        gReportManager.Log("Warning", "No point setting a timer if you don't wait for it to finish. " +
                                    std::to_string(seconds) + " second timer request ignored.");
        ExecError();
    }
    else
    {
        Timers::AddTimerSeconds(seconds, currentThread->AddWait());
    }
    return 0;
}
RegFunc1(SetTimerSeconds, void, float, WAITABLE, REL_FUNC);

float GetTimeMultiplier()
{
    return GEngine::Instance()->GetTimeMultiplier();
}
RegFunc0(GetTimeMultiplier, float, IMMEDIATE, REL_FUNC);

shpvoid SetTimeMultiplier(float multiplier)
{
    GEngine::Instance()->SetTimeMultiplier(multiplier);
    return 0;
}
RegFunc1(SetTimeMultiplier, void, float, IMMEDIATE, REL_FUNC);

float GetRandomFloat(float lower, float upper)
{
    return Random::Range(lower, upper);
}
RegFunc2(GetRandomFloat, float, float, float, IMMEDIATE, REL_FUNC);

int GetRandomInt(int lower, int upper)
{
    // Both upper and lower are inclusive, according to docs.
    // So, add one to upper since our range function is upper-exclusive.
    return Random::Range(lower, upper + 1);
}
RegFunc2(GetRandomInt, int, int, int, IMMEDIATE, REL_FUNC);
