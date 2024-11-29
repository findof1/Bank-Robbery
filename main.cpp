#define SDL_MAIN_HANDLED
#include "game.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main()
{
    Game game;
    game.run();
    return 0;
}