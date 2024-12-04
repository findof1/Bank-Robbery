#pragma once
#include "globals.h"
#include "types.h"
#include "stb_image.h"
#include <iostream>
#include <fstream>
#include <string>

void loadTextures()
{
  for (const auto &filepath : textureFilepaths)
  {
    Texture tex;
    tex.data = stbi_load(filepath.c_str(), &tex.width, &tex.height, &tex.channels, 4);
    if (!tex.data)
    {
      std::cerr << "Failed to load texture: " << filepath << std::endl;
      continue;
    }
    loadedTextures.push_back(tex);
  }
}

int getCell(int x, int y)
{
  if (x >= 0 && x < mapX && y >= 0 && y < mapY)
  {
    int cellIndex = y * mapX + x;
    return cellIndex;
  }
  else
  {
    return -1;
  }
}

void deserialize(const std::string &filename)
{
  std::ifstream file(filename, std::ios::binary | std::ios::in);
  if (file)
  {
    file.read(reinterpret_cast<char *>(&mapX), sizeof(int));
    file.read(reinterpret_cast<char *>(&mapY), sizeof(int));
    maxDepth = std::max(mapX, mapY);

    size_t count;
    file.read(reinterpret_cast<char *>(&count), sizeof(count));
    map.resize(count);
    file.read(reinterpret_cast<char *>(map.data()), sizeof(int) * count);

    count;
    file.read(reinterpret_cast<char *>(&count), sizeof(count));
    mapFloors.resize(count);
    file.read(reinterpret_cast<char *>(mapFloors.data()), sizeof(int) * count);

    count;
    file.read(reinterpret_cast<char *>(&count), sizeof(count));
    mapCeiling.resize(count);
    file.read(reinterpret_cast<char *>(mapCeiling.data()), sizeof(int) * count);
    file.close();
  }
  else
  {
    std::cerr << "Error opening file for reading.\n";
  }

  for (int x = 0; x < mapX; x++)
  {
    for (int y = 0; y < mapY; y++)
    {
      if (mapFloors[getCell(x, y)] == 18)
      {
        float gridWidth = 16;
        float gridHeight = 16;

        for (int i = 0; i < 3; i++)
        {
          for (int j = 0; j < 3; j++)
          {
            Sprite spike;
            spike.x = i * gridWidth + x * cellWidth + 16;
            spike.y = j * gridHeight + y * cellWidth + 16;
            spike.type = Spike;
            spike.enemyLastMeleeTime = std::chrono::high_resolution_clock::now();
            spike.enemyLastBulletTime = std::chrono::high_resolution_clock::now();
            spike.z = 19;
            sprites.emplace_back(spike);
          }
        }
      }
    }
  }
}

void deserializeSprites(const std::string &filename)
{
  std::ifstream file(filename, std::ios::binary | std::ios::in);
  if (file)
  {
    int spritesSize;
    file.read(reinterpret_cast<char *>(&spritesSize), sizeof(int));
    for (int i = 0; i < spritesSize; i++)
    {
      bool invalid = false;
      Sprite sprite;
      int type;
      file.read(reinterpret_cast<char *>(&type), sizeof(int));
      if (type - 1 < 0 || type - 1 > static_cast<int>(SpriteType::GoldBar))
      {
        invalid = true;
      }
      sprite.type = static_cast<SpriteType>(type - 1);

      file.read(reinterpret_cast<char *>(&sprite.x), sizeof(float));
      file.read(reinterpret_cast<char *>(&sprite.y), sizeof(float));
      file.read(reinterpret_cast<char *>(&sprite.z), sizeof(float));
      if (sprite.type == Enemy || sprite.type == ShooterEnemy || sprite.type == HammerEnemy)
      {
        sprite.z = 20;
      }
      file.read(reinterpret_cast<char *>(&sprite.scaleX), sizeof(float));
      file.read(reinterpret_cast<char *>(&sprite.scaleY), sizeof(float));
      file.read(reinterpret_cast<char *>(&sprite.active), sizeof(bool));

      bool hasHealth;
      file.read(reinterpret_cast<char *>(&hasHealth), sizeof(bool));
      if (sprite.type == Enemy && hasHealth == false)
      {
        sprite.health = 20;
      }
      if (sprite.type == HammerEnemy && hasHealth == false)
      {
        sprite.health = 50;
      }
      if (sprite.type == DroneEnemy && hasHealth == false)
      {
        sprite.health = 1;
      }
      if (sprite.type == ShooterEnemy && hasHealth == false)
      {
        sprite.health = 5;
      }
      if (sprite.type == ShooterEnemy)
      {
        sprite.enemyLastBulletTime = std::chrono::high_resolution_clock::now();
        sprite.enemyLastMeleeTime = std::chrono::high_resolution_clock::now();
        sprite.move = false;
      }
      if (sprite.type == Enemy)
      {
        sprite.enemyLastMeleeTime = std::chrono::high_resolution_clock::now();
        sprite.move = false;
      }
      if (sprite.type == DroneEnemy)
      {
        sprite.move = false;
      }
      if (sprite.type == Spike)
      {
        sprite.enemyLastMeleeTime = std::chrono::high_resolution_clock::now();
        sprite.enemyLastBulletTime = std::chrono::high_resolution_clock::now();
        sprite.z = 19;
      }
      if (sprite.type == HammerEnemy)
      {
        sprite.scaleX = 1.2;
        sprite.scaleY = 1.2;
        sprite.enemyLastMeleeTime = std::chrono::high_resolution_clock::now();
        sprite.move = false;
      }

      if (hasHealth)
      {
        file.read(reinterpret_cast<char *>(&sprite.health), sizeof(float));
      }
      bool hasDirection;
      file.read(reinterpret_cast<char *>(&hasDirection), sizeof(bool));
      if (hasDirection)
      {
        file.read(reinterpret_cast<char *>(&sprite.direction), sizeof(float));
      }

      if (!invalid)
        sprites.emplace_back(sprite);
    }
    file.close();
  }
  else
  {
    std::cerr << "Error opening file for reading.\n";
  }
}

float FixAngle(float a)
{
  if (a > 359)
  {
    a -= 360;
  }
  if (a < 0)
  {
    a += 360;
  }
  return a;
}

void getRGBFromTexture(int hitType, int x, int y, uint8_t &r, uint8_t &g, uint8_t &b)
{
  if (hitType < 1 || hitType > loadedTextures.size())
  {
    // std::cerr << "Invalid hitType: " << hitType << std::endl;
    return;
  }

  const Texture &tex = loadedTextures[hitType - 1];
  if (x < 0 || x >= tex.width || y < 0 || y >= tex.height)
  {
    // std::cerr << "Coordinates out of bounds: " << x << ", " << y << std::endl;
    return;
  }

  int index = (y * tex.width + x) * 4;
  r = tex.data[index];
  g = tex.data[index + 1];
  b = tex.data[index + 2];
}

void getRGBFromTexture(int hitType, int x, int y, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a)
{
  if (hitType < 1 || hitType > loadedTextures.size())
  {
    // std::cerr << "Invalid hitType: " << hitType << std::endl;
    return;
  }

  const Texture &tex = loadedTextures[hitType - 1];
  if (x < 0 || x >= tex.width || y < 0 || y >= tex.height)
  {
    // std::cerr << "Coordinates out of bounds: " << x << ", " << y << std::endl;
    return;
  }

  int index = (y * tex.width + x) * 4;
  r = tex.data[index];
  g = tex.data[index + 1];
  b = tex.data[index + 2];
  a = tex.data[index + 3];
}

float degToRad(float angle) { return angle * M_PI / 180.0; }

SDL_Texture *loadImage(SDL_Window *window, SDL_Renderer *renderer, std::string filepath)
{
  SDL_Texture *image = IMG_LoadTexture(renderer, filepath.c_str());
  if (!image)
  {
    SDL_Log("Unable to load image: %s", IMG_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    exit(EXIT_FAILURE);
  }
  return image;
}

void loadSound(std::string filepath)
{
  Mix_Chunk *sound = Mix_LoadWAV(filepath.c_str());
  if (!sound)
  {
    std::cerr << "Failed to load sound: " << Mix_GetError() << "\n";
    exit(EXIT_FAILURE);
  }
  Mix_VolumeChunk(sound, 64);
  sounds.emplace_back(sound);
}

TTF_Font *loadFont(std::string filepath, int size = 24)
{
  TTF_Font *font = TTF_OpenFont(filepath.c_str(), size);
  if (!font)
  {
    printf("Error loading font: %s\n", TTF_GetError());
    exit(EXIT_FAILURE);
  }
  return font;
}