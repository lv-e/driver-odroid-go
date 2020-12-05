
#include "lv-game/scene_game.h"
#include "lv-game/scene_main.h"


// this method should be called by drivers
// before starting the engine
// will do scene mapping internals
static void setupScenes() {
    scene_game_setup();
    scene_main_setup();
}

