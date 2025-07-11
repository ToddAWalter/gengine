#include "SheepAPI_Rendering.h"

#include "ActionManager.h"
#include "VideoHelper.h"

using namespace std;

shpvoid PlayFullScreenMovie(const std::string& movieName)
{
    // Don't start a video during action skip. The game doesn't handle this well.
    //TODO: Double check how original game handles this.
    if(!gActionManager.IsSkippingCurrentAction())
    {
        VideoHelper::PlayVideoWithCaptions(movieName, AddWait());
    }
    return 0;
}
RegFunc1(PlayFullScreenMovie, void, string, WAITABLE, REL_FUNC);

shpvoid PlayFullScreenMovieX(const std::string& movieName, int autoclose)
{
    if(!gActionManager.IsSkippingCurrentAction())
    {
        VideoHelper::PlayVideoWithCaptions(movieName, true, autoclose != 0 ? true : false, AddWait());
    }
    return 0;
}
RegFunc2(PlayFullScreenMovieX, void, string, int, WAITABLE, REL_FUNC);

shpvoid PlayMovie(const std::string& movieName)
{
    if(!gActionManager.IsSkippingCurrentAction())
    {
        VideoHelper::PlayVideoWithCaptions(movieName, false, true, AddWait());
    }
    return 0;
}
RegFunc1(PlayMovie, void, string, WAITABLE, REL_FUNC);
