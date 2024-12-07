#pragma once
#include <optional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <string>

struct PlayerData
{
  bool pistolUpgraded = false;
  bool shotgunUnlocked = false;
  bool shotgunUpgraded = false;
  bool minigunUnlocked = false;
  bool minigunUpgraded = false;
  int money = 0;
};

enum GunType
{
  Pistol,
  Shotgun,
  Minigun
};

struct Texture
{
  int width, height, channels;
  unsigned char *data;
};

struct Player
{
  glm::vec2 pos;
  float angle;
  float FOV;
};

enum SpriteType
{
  Key,
  Bomb,
  Enemy,
  ShooterEnemy,
  Bullet,
  EnemyBullet,
  Coin,
  HammerEnemy,
  Spike,
  DroneEnemy,
  GoldBar,
  Swat
};

struct Sprite
{
  SpriteType type;
  float x, y, z;
  float scaleX = 1;
  float scaleY = 1;
  bool active;
  std::optional<float> direction;
  std::optional<float> health;
  std::optional<std::chrono::_V2::system_clock::time_point> enemyLastBulletTime;
  std::optional<std::chrono::_V2::system_clock::time_point> enemyLastMeleeTime;
  std::optional<bool> move;
  std::optional<int> soundChannel;
};

class Button
{
private:
  SDL_FRect rect;
  SDL_Color color;
  SDL_Color borderColor;
  float borderThickness;
  SDL_Color hoverColor;
  SDL_Color currentColor;
  SDL_Renderer *renderer;
  SDL_Texture *textTexture;
  SDL_Rect textRect;
  float textScale;

public:
  Button(SDL_Renderer *renderer, float x, float y, float w, float h, SDL_Color color, SDL_Color hoverColor, const std::string &text, TTF_Font *font, float borderThickness, SDL_Color borderColor, float textScale = 1)
      : renderer(renderer), color(color), hoverColor(hoverColor), currentColor(color), borderThickness(borderThickness), borderColor(borderColor), textScale(textScale)
  {
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;

    SDL_Surface *textSurface = TTF_RenderText_Solid(font, text.c_str(), {25, 25, 25, 255});
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    textRect = {static_cast<int>(x + (w - textSurface->w * textScale) / 2),
                static_cast<int>(y + (h - textSurface->h * textScale) / 2),
                static_cast<int>(textSurface->w * textScale),
                static_cast<int>(textSurface->h * textScale)};

    SDL_FreeSurface(textSurface);
  }

  ~Button()
  {
    SDL_DestroyTexture(textTexture);
  }

  void render()
  {

    SDL_FRect borderRect = {rect.x - borderThickness, rect.y - borderThickness,
                            rect.w + 2 * borderThickness, rect.h + 2 * borderThickness};
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderFillRectF(renderer, &borderRect);

    SDL_FRect highlightRect;
    highlightRect.x = rect.x - 5;
    highlightRect.y = rect.y - 5;
    highlightRect.w = rect.w + 10;
    highlightRect.h = rect.h + 10;
    SDL_SetRenderDrawColor(renderer, 235, 235, 235, 100);
    SDL_RenderFillRectF(renderer, &highlightRect);

    SDL_FRect shadowRect;
    shadowRect.x = rect.x + 5;
    shadowRect.y = rect.y + 5;
    shadowRect.w = rect.w;
    shadowRect.h = rect.h;
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 100);
    SDL_RenderFillRectF(renderer, &shadowRect);

    SDL_SetRenderDrawColor(renderer, currentColor.r, currentColor.g, currentColor.b, currentColor.a);
    SDL_RenderFillRectF(renderer, &rect);

    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
  }

  bool handleEvent(const SDL_Event &event)
  {
    if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN)
    {
      int mouseX, mouseY;
      SDL_GetMouseState(&mouseX, &mouseY);
      SDL_FPoint mousePos = {static_cast<float>(mouseX), static_cast<float>(mouseY)};
      bool isHovered = SDL_PointInFRect(&mousePos, &rect);

      if (isHovered)
      {
        currentColor = hoverColor;

        if (event.type == SDL_MOUSEBUTTONDOWN)
        {
          return true;
        }
      }
      else
      {
        currentColor = color;
      }
    }

    return false;
  }
};