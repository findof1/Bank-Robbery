#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "types.h"
#include <random>

class Game
{
public:
  Game();
  void run();

private:
  Player player;
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Color healthTextColor = {155, 25, 25, 255};
  SDL_Color coinTextColor = {255, 255, 25, 255};
  SDL_Texture *background;
  SDL_Texture *crosshair;
  SDL_Texture *pistol;
  SDL_Texture *shotgun;
  SDL_Texture *minigun;
  SDL_Texture *titleText;
  SDL_Rect titleRect;

  TTF_Font *font;

  void initSDL();
  SDL_Window *initWindow();
  SDL_Renderer *initRenderer(SDL_Window *window);
  void initIcon(SDL_Window *window);
  void raycast(SDL_Renderer *renderer);

  void handleSprites(SDL_Renderer *renderer);
  int getSpriteTextureIndex(SpriteType type);
  void handleEnemyBullet(int i);
  void handleBullet(int i);
  void handleEnemyMovement(int i);
  void handleShooterEnemy(int i);
  void handleSwatBoss(int i);
  void renderSprite(int i);

  void shootBullet();
  void handleInput();
  void displayMainMenu(SDL_Renderer *renderer, TTF_Font *font,
                       SDL_Texture *background, SDL_Texture *titleText, SDL_Rect titleRect);
  void displayShop(SDL_Renderer *renderer, TTF_Font *font, SDL_Texture *pistol, SDL_Texture *shotgun,
                   SDL_Texture *minigun, SDL_Color coinTextColor);
};