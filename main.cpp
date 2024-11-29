#define SDL_MAIN_HANDLED
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "game.h"

int main()
{
    Game game;
    game.run();
    return 0;
}