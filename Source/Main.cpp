//
// Clark Kromenaker
//
// Program point of entry for all platforms.
//
#include <cstdio>

#define SDL_MAIN_HANDLED // For Windows: we provide our own main, so use that!
#include "BuildEnv.h"
#include "GEngine.h"

int main(int argc, const char* argv[])
{
    printf("--------------------\n");
    printf("GEngine v%s\n", PROJECT_VERSION);
    printf("--------------------\n");

    // Create the engine.
    GEngine engine;
    
    // If init succeeds, we can "run" the engine.
    // If init fails, the program ends immediately. Failing code will output an error of some kind.
    bool initSucceeded = engine.Initialize();
    if(initSucceeded)
    {
        engine.Run();
    }
    
    // Do the opposite of init and shut...it...down.
    engine.Shutdown();
    return 0;
}
