#include "game.h"
#include <iostream>
#include "utils.h"
#include <optional>
#include <algorithm>
#include "types.h"
#include "globals.h"

Game::Game()
{
  loadTextures();
  initSDL();
  window = initWindow();
  initIcon(window);
  renderer = initRenderer(window);

  loadSound("./sounds/pickupCoin.wav");
  loadSound("./sounds/shoot.wav");
  loadSound("./sounds/explosion.wav");
  loadSound("./sounds/step.wav");
  loadSound("./sounds/song.wav");

  font = loadFont("C:\\Windows\\Fonts\\courbd.ttf");

  background = loadImage(window, renderer, "./textures/background.png");
  SDL_SetTextureBlendMode(background, SDL_BLENDMODE_BLEND);
  SDL_SetTextureAlphaMod(background, 100);

  crosshair = loadImage(window, renderer, "./textures/crosshair.png");
  pistol = loadImage(window, renderer, "./textures/pistol.png");
  shotgun = loadImage(window, renderer, "./textures/shotgun.png");
  minigun = loadImage(window, renderer, "./textures/minigun.png");

  std::string title = "It's a Bank Robbery";
  SDL_Surface *titleTextSurface = TTF_RenderText_Solid(font, title.c_str(), {255, 255, 255, 255});
  if (!titleTextSurface)
  {
    printf("Error rendering text: %s\n", TTF_GetError());
    exit(EXIT_FAILURE);
  }
  titleText = SDL_CreateTextureFromSurface(renderer, titleTextSurface);
  titleRect = {512 - ((titleTextSurface->w * 3) / 2), 60, titleTextSurface->w * 3, titleTextSurface->h * 3};
  SDL_FreeSurface(titleTextSurface);

  player = {{80.0f, 80.0f}, 0.0f, 60};

  deserializePlayer("save.dat");

  Achievements::update();
}

void Game::run()
{
  auto startTime = std::chrono::high_resolution_clock::now();
  lastTime = std::chrono::high_resolution_clock::now();
  while (gameRunning)
  {
    if (musicChannel == -1)
    {
      musicChannel = Mix_PlayChannel(-1, sounds.at(4), 0);
    }
    if (!Mix_Playing(musicChannel))
    {
      musicChannel = Mix_PlayChannel(-1, sounds.at(4), 0);
    }

    distances.clear();
    currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed = currentTime - startTime;
    deltaTime = ((std::chrono::duration<float>)(currentTime - lastTime)).count();

    lastTime = currentTime;

    if (level == 0)
    {
      displayMainMenu(renderer, font, background, titleText, titleRect);
      continue;
    }
    else if (level == -1)
    {
      displayShop(renderer, font, pistol, shotgun, minigun, coinTextColor);
      continue;
    }
    else if (level == -2)
    {
      displayAchievements(renderer, font);
      continue;
    }
    if (health <= 0)
    {
      level = 0;
      continue;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        gameRunning = false;
      }
    }

    handleInput();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_Rect bottomBackground;
    bottomBackground.x = 0;
    bottomBackground.h = 256;
    bottomBackground.y = 256;
    bottomBackground.w = 1024;
    SDL_RenderFillRect(renderer, &bottomBackground);

    SDL_SetRenderDrawColor(renderer, 51, 197, 255, 255);
    SDL_Rect topBackground;
    topBackground.x = 0;
    topBackground.h = 256;
    topBackground.y = 0;
    topBackground.w = 1024;
    SDL_RenderFillRect(renderer, &topBackground);

    raycast(renderer);

    handleSprites(renderer);

    std::string text = "Health: " + std::to_string(health);
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, text.c_str(), healthTextColor);
    if (!textSurface)
    {
      printf("Error rendering text: %s\n", TTF_GetError());
      exit(EXIT_FAILURE);
    }

    SDL_Texture *healthText = SDL_CreateTextureFromSurface(renderer, textSurface);

    SDL_Rect textRect = {20, 20, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, healthText, NULL, &textRect);

    text = "Money: " + std::to_string(levelMoney + playerData.money);
    textSurface = TTF_RenderText_Solid(font, text.c_str(), coinTextColor);
    if (!textSurface)
    {
      printf("Error rendering text: %s\n", TTF_GetError());
      exit(EXIT_FAILURE);
    }

    SDL_Texture *coinText = SDL_CreateTextureFromSurface(renderer, textSurface);

    textRect = {1024 - textSurface->w - 20, 20, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, coinText, NULL, &textRect);

    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(healthText);
    SDL_DestroyTexture(coinText);

    SDL_Rect crosshairRect = {512 - 5, 256 - 5, 10, 10};
    SDL_RenderCopy(renderer, crosshair, NULL, &crosshairRect);

    SDL_RenderPresent(renderer);

    SDL_Delay(16);
  }
  serializePlayer("save.dat");

  for (auto sound : sounds)
  {
    Mix_FreeChunk(sound);
  }
  Mix_CloseAudio();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void Game::initSDL()
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() != 0)
  {
    std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
    exit(EXIT_FAILURE);
  }
  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
  {
    SDL_Log("Unable to initialize SDL_image: %s", IMG_GetError());
    SDL_Quit();
    exit(EXIT_FAILURE);
  }
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
  {
    std::cerr << "Failed to initialize SDL_mixer: " << Mix_GetError() << "\n";
    exit(EXIT_FAILURE);
  }
}

SDL_Window *Game::initWindow()
{
  SDL_Window *window = SDL_CreateWindow("It's a Bank Robbery", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 512, SDL_WINDOW_SHOWN);
  if (!window)
  {
    std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    exit(EXIT_FAILURE);
  }
  return window;
}

SDL_Renderer *Game::initRenderer(SDL_Window *window)
{
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer)
  {
    std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    exit(EXIT_FAILURE);
  }
  return renderer;
}

void Game::initIcon(SDL_Window *window)
{
  SDL_Surface *icon = IMG_Load("./textures/icon.png");
  if (icon)
  {
    SDL_SetWindowIcon(window, icon);
    SDL_FreeSurface(icon);
  }
  else
  {
    SDL_Log("Unable to load icon: %s", SDL_GetError());
  }
}

void Game::raycast(SDL_Renderer *renderer)
{
  float rayAngle = FixAngle(player.angle - (player.FOV / 2));

  for (float i = 0; i < player.FOV; i += rayStep)
  {
    float rayX = player.pos.x;
    float rayY = player.pos.y;
    float dy;
    float dx;

    float distanceHorizontal = 10000000;
    int cellIndexX;
    int cellIndexY = floor(rayY / cellWidth);
    int depth = 0;

    int mappedPosHorizontal;
    int hitTypeHorizontal;

    while (depth < maxDepth)
    {

      if (sin(degToRad(rayAngle)) > 0)
      {
        dy = cellWidth - (rayY - (cellWidth * cellIndexY));
        if (dy == 0)
        {
          dy = cellWidth;
        }
      }
      else
      {
        dy = -(rayY - (cellWidth * cellIndexY));
        if (dy == 0)
        {
          dy = -cellWidth;
        }
      }
      dx = dy / tan(degToRad(rayAngle));

      rayY = rayY + dy;
      rayX = rayX + dx;
      cellIndexX = floor(rayX / cellWidth);
      cellIndexY = floor(rayY / cellWidth);

      int mapCellIndex = getCell(cellIndexX, cellIndexY);

      if (mapCellIndex == -1)
      {
        depth = maxDepth;
      }
      if (map[mapCellIndex] != 0)
      {
        hitTypeHorizontal = map[mapCellIndex];
        depth = maxDepth;
        mappedPosHorizontal = static_cast<int>((rayX - cellIndexX * cellWidth) / 2.0f);
        distanceHorizontal = sqrt(pow(rayX - player.pos.x, 2) + pow(rayY - player.pos.y, 2));
      }
      if (map[getCell(cellIndexX, cellIndexY - 1)] != 0)
      {
        hitTypeHorizontal = map[getCell(cellIndexX, cellIndexY - 1)];
        depth = maxDepth;
        mappedPosHorizontal = static_cast<int>((rayX - cellIndexX * cellWidth) / 2.0f);
        distanceHorizontal = sqrt(pow(rayX - player.pos.x, 2) + pow(rayY - player.pos.y, 2));
      }
      depth++;
    }

    float horizontalRayX = rayX;
    float horizontalRayY = rayY;
    // vertical

    int mappedPosVertical;
    int hitTypeVertical;

    rayX = player.pos.x;
    rayY = player.pos.y;

    float distanceVertical = 10000000;

    cellIndexX = floor(rayX / cellWidth);

    depth = 0;
    while (depth < maxDepth)
    {

      if (cos(degToRad(rayAngle)) > 0)
      {
        dx = cellWidth - (rayX - (cellWidth * cellIndexX));
        if (dx == 0)
        {
          dx = cellWidth;
        }
      }
      else
      {
        dx = -(rayX - (cellWidth * cellIndexX));
        if (dx == 0)
        {
          dx = -cellWidth;
        }
      }
      dy = dx / (1 / tan(degToRad(rayAngle)));

      rayY = rayY + dy;
      rayX = rayX + dx;
      cellIndexX = floor(rayX / cellWidth);
      cellIndexY = floor(rayY / cellWidth);
      int mapCellIndex = getCell(cellIndexX, cellIndexY);

      if (mapCellIndex == -1)
      {
        depth = maxDepth;
      }

      if (map[mapCellIndex] != 0)
      {
        hitTypeVertical = map[mapCellIndex];
        depth = maxDepth;
        mappedPosVertical = static_cast<int>((rayY - cellIndexY * cellWidth) / 2.0f);
        distanceVertical = sqrt(pow(rayX - player.pos.x, 2) + pow(rayY - player.pos.y, 2));
      }
      if (map[getCell(cellIndexX - 1, cellIndexY)] != 0)
      {
        hitTypeVertical = map[getCell(cellIndexX - 1, cellIndexY)];
        depth = maxDepth;
        mappedPosVertical = static_cast<int>((rayY - cellIndexY * cellWidth) / 2.0f);
        distanceVertical = sqrt(pow(rayX - player.pos.x, 2) + pow(rayY - player.pos.y, 2));
      }
      depth++;
    }

    /*
SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
SDL_RenderDrawLine(renderer, player.pos.x, player.pos.y, horizontalRayX, horizontalRayY);
SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
SDL_RenderDrawLine(renderer, player.pos.x, player.pos.y, rayX, rayY);
*/
    int mappedPos;
    int hitType;
    // SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    if (distanceVertical < distanceHorizontal)
    {
      // SDL_RenderDrawLine(renderer, player.pos.x, player.pos.y, horizontalRayX, horizontalRayY);
      mappedPos = mappedPosVertical;
      hitType = hitTypeVertical;
      SDL_SetRenderDrawColor(renderer, 0, 155, 0, 255);
    }
    else
    {
      mappedPos = mappedPosHorizontal;
      hitType = hitTypeHorizontal;
      SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
      // SDL_RenderDrawLine(renderer, player.pos.x, player.pos.y, rayX, rayY);
    }

    float distance = std::min(distanceHorizontal, distanceVertical);
    float correctedDistance = distance * cos(degToRad(FixAngle(player.angle - rayAngle)));
    distances.emplace_back(distance);
    SDL_FRect rectangle;
    rectangle.x = i * (1024 / (player.FOV));
    rectangle.h = (64 * 512) / correctedDistance;
    rectangle.y = (512 / 2) - (rectangle.h / 2);
    rectangle.w = (1024 / (player.FOV)) * rayStep;
    // SDL_RenderFillRect(renderer, &rectangle);

    float smallRectHeight = rectangle.h / 32;

    for (int j = 0; j < 32; j++)
    {
      Uint8 r, g, b;
      getRGBFromTexture(hitType, mappedPos, j, r, g, b);
      SDL_SetRenderDrawColor(renderer, r, g, b, 255);
      float smallRectY = rectangle.y + j * smallRectHeight;

      SDL_FRect smallRect = rectangle;
      smallRect.y = smallRectY;
      smallRect.h = smallRectHeight;

      SDL_RenderFillRectF(renderer, &smallRect);
    }

    float deg = -degToRad(rayAngle);
    float rayAngleFix = cos(degToRad(FixAngle(player.angle - rayAngle)));
    float drawX = i * (1024 / (player.FOV));
    float drawWidth = (1024 / (player.FOV)) * rayStep;
    for (int y = rectangle.y + rectangle.h; y < 512; y += drawWidth / 1.5)
    {
      float dy = y - (512 / 2.0);
      float textureX = player.pos.x / 2 + cos(deg) * 126 * 2 * 32 / dy / rayAngleFix;
      float textureY = player.pos.y / 2 - sin(deg) * 126 * 2 * 32 / dy / rayAngleFix;
      int textureType = mapFloors[(int)(textureY / 32.0) * mapX + (int)(textureX / 32.0)];
      if (textureType != 0)
      {
        uint8_t r, g, b;
        getRGBFromTexture(textureType, (int)(textureX) % 32, (int)(textureY) % 32, r, g, b);
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_FRect rectangle;
        rectangle.x = drawX;
        rectangle.h = drawWidth;
        rectangle.y = y;
        rectangle.w = drawWidth;
        SDL_RenderFillRectF(renderer, &rectangle);
      }
      textureX = player.pos.x / 2 + cos(deg) * 126 * 2 * 32 / dy / rayAngleFix;
      textureY = player.pos.y / 2 - sin(deg) * 126 * 2 * 32 / dy / rayAngleFix;
      textureType = mapCeiling[(int)(textureY / 32.0) * mapX + (int)(textureX / 32.0)];
      if (textureType != 0)
      {
        Uint8 r, g, b;
        getRGBFromTexture(textureType, (int)(textureX) % 32, (int)(textureY) % 32, r, g, b);
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_FRect rectangle;
        rectangle.x = drawX;
        rectangle.h = drawWidth;
        rectangle.y = 512 - y;
        rectangle.w = drawWidth;
        SDL_RenderFillRectF(renderer, &rectangle);
      }
    }

    rayAngle = FixAngle(rayAngle + rayStep);
  }
}

void Game::handleSprites(SDL_Renderer *renderer)
{
  glm::vec2 playerPos(player.pos.x, player.pos.y);
  std::sort(sprites.begin(), sprites.end(),
            [playerPos](const Sprite &a, const Sprite &b)
            {
              return glm::distance(glm::vec2(a.x, a.y), playerPos) > glm::distance(glm::vec2(b.x, b.y), playerPos);
            });

  for (int i = 0; i < sprites.size(); i++)
  {
    if (sprites[i].type == Spike && sprites[i].active == false)
    {
      if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sprites[i].enemyLastBulletTime.value()).count() > spikeTrapInterval)
      {
        sprites[i].active = true;
        sprites[i].enemyLastBulletTime = std::chrono::high_resolution_clock::now();
      }
    }

    if (sprites[i].active == false)
      continue;

    if (sprites[i].type == Spike)
    {
      if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sprites[i].enemyLastBulletTime.value()).count() > spikeTrapInterval)
      {
        sprites[i].active = false;
        sprites[i].enemyLastBulletTime = std::chrono::high_resolution_clock::now();
      }

      int cellIndexX = floor(sprites[i].x / cellWidth);
      int cellIndexY = floor(sprites[i].y / cellWidth);
      int playerCellIndexX = floor(player.pos.x / cellWidth);
      int playerCellIndexY = floor(player.pos.y / cellWidth);
      if (cellIndexX == playerCellIndexX && cellIndexY == playerCellIndexY && std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sprites[i].enemyLastMeleeTime.value()).count() > 5000)
      {
        sprites[i].enemyLastMeleeTime = std::chrono::high_resolution_clock::now();
        health -= 1;
      }
    }

    if (sprites[i].type == Key)
    {
      float distance = sqrt(pow(sprites[i].x - player.pos.x, 2) + pow(sprites[i].y - player.pos.y, 2));
      if (distance < 15)
      {
        sprites[i].active = false;
        keyCount += 1;
      }
    }

    if (sprites[i].type == Coin)
    {
      float distance = sqrt(pow(sprites[i].x - player.pos.x, 2) + pow(sprites[i].y - player.pos.y, 2));
      if (distance < 15)
      {
        Mix_PlayChannel(-1, sounds.at(0), 0);
        sprites[i].active = false;
        levelMoney += 5;
      }
    }

    if (sprites[i].type == GoldBar)
    {
      float distance = sqrt(pow(sprites[i].x - player.pos.x, 2) + pow(sprites[i].y - player.pos.y, 2));
      if (distance < 15)
      {
        Mix_PlayChannel(-1, sounds.at(0), 0);
        sprites[i].active = false;
        levelMoney += 100;
      }
    }

    if (sprites[i].type == Bomb)
    {
      float distance = sqrt(pow(sprites[i].x - player.pos.x, 2) + pow(sprites[i].y - player.pos.y, 2));
      if (distance < 15)
      {
        sprites[i].active = false;
        bombCount += 1;
      }
    }

    if (sprites[i].type == Bullet)
    {
      handleBullet(i);
      if (sprites[i].active == false)
        continue;
    }

    if (sprites[i].type == EnemyBullet)
    {
      handleEnemyBullet(i);
      if (sprites[i].active == false)
        continue;
    }

    if ((sprites[i].type == Enemy || sprites[i].type == ShooterEnemy || sprites[i].type == HammerEnemy || sprites[i].type == DroneEnemy) && sprites[i].move == true)
    {
      handleEnemyMovement(i);
      if (sprites[i].active == false)
        continue;
    }

    if (sprites[i].type == Swat && sprites[i].move == true)
    {
      handleSwatBoss(i);
    }

    if (sprites[i].type == ShooterEnemy && std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sprites[i].enemyLastBulletTime.value()).count() > enemyShootingCooldown && sprites[i].move == true)
    {
      handleShooterEnemy(i);
    }

    renderSprite(i);

    if (sprites[i].type == Swat && sprites[i].move == true)
    {
      renderHealthBar(renderer, sprites[i].health.value() / BossValues::initialBossHealth, font);
    }
  }
}

void Game::renderSprite(int i)
{
  float spriteX = sprites[i].x - player.pos.x;
  float spriteY = sprites[i].y - player.pos.y;
  float spriteZ = sprites[i].z;

  float angleRad = -degToRad(player.angle);
  float rotatedX = spriteY * cos(angleRad) + spriteX * sin(angleRad);
  float rotatedY = spriteX * cos(angleRad) - spriteY * sin(angleRad);

  if (rotatedY > 0)
  {

    float fovFactor = 512.0f / tan(degToRad(player.FOV / 2));

    float projectedX = (rotatedX * fovFactor / rotatedY) + (1024 / 2);
    float projectedY = (spriteZ * fovFactor / rotatedY) + (512 / 2);

    float distance = sqrt(pow(spriteX, 2) + pow(spriteY, 2));

    float preCalculatedWidth = ((1024 / (player.FOV)) * rayStep + (1024.f / distance)) * 0.45 * sprites[i].scaleX;
    float preCalculatedHeight = ((1024 / (player.FOV)) * rayStep + (1024.f / distance)) * 0.45 * sprites[i].scaleX;

    int textureIndex = getSpriteTextureIndex(sprites[i].type);

    for (int x = 0; x < loadedTextures[textureIndex].width; x++)
    {
      float recX = projectedX + ((x * (256 * sprites[i].scaleX)) / distance);

      recX -= ((preCalculatedWidth * loadedTextures[textureIndex].width) / 8);

      if (static_cast<int>(glm::clamp((recX * (player.FOV / rayStep)) / 1024, 0.f, (player.FOV / rayStep))) - 1 >= 0 && static_cast<int>(glm::clamp((recX * (player.FOV / rayStep)) / 1024, 0.f, (player.FOV / rayStep))) - 1 <= (player.FOV / rayStep) && distance < distances.at(static_cast<int>(glm::clamp((recX * (player.FOV / rayStep)) / 1024, 0.f, (player.FOV / rayStep))) - 1))
      {

        if ((sprites[i].type == Enemy || sprites[i].type == ShooterEnemy || sprites[i].type == HammerEnemy || sprites[i].type == DroneEnemy || sprites[i].type == Swat) && sprites[i].move == false && recX < 1024)
        {
          float deltaX = player.pos.x - sprites[i].x;
          float deltaY = player.pos.y - sprites[i].y;

          sprites[i].move = true;
        }

        for (int y = 0; y < loadedTextures[textureIndex].height; y++)
        {
          Uint8 r, g, b, a;
          getRGBFromTexture(textureIndex + 1, x, loadedTextures[textureIndex].height - 1 - y, r, g, b, a);

          SDL_SetRenderDrawColor(renderer, r, g, b, 255);
          if (a != 0)
          {
            SDL_FRect rectangle;
            rectangle.x = recX;
            rectangle.y = projectedY - ((y * (256 * sprites[i].scaleY)) / distance);
            rectangle.w = preCalculatedWidth;
            rectangle.h = preCalculatedHeight;
            SDL_RenderFillRectF(renderer, &rectangle);
          }
        }
      }
    }
  }
}

int Game::getSpriteTextureIndex(SpriteType type)
{

  if (type == Enemy || type == ShooterEnemy)
    return 20;

  if (type == Bomb)
    return 21;

  if (type == Bullet || type == EnemyBullet)
    return 22;

  if (type == Key)
    return 23;

  if (type == Coin)
    return 24;

  if (type == HammerEnemy)
    return 25;

  if (type == Spike)
    return 26;

  if (type == DroneEnemy)
    return 27;

  if (type == GoldBar)
    return 28;

  if (type == Swat)
    return 29;

  std::cout << "Invalid sprite type when getting texture" << std::endl;
  return -1;
}

void Game::handleSwatBoss(int i)
{
  if (!sprites[i].soundChannel.has_value())
  {
    sprites[i].soundChannel = Mix_PlayChannel(-1, sounds.at(3), 0);
  }
  if (!Mix_Playing(sprites[i].soundChannel.value()))
  {
    sprites[i].soundChannel = Mix_PlayChannel(-1, sounds.at(3), 0);
  }
  if (sprites[i].health <= 0)
  {
    levelMoney += 100;
    sprites[i].active = false;
    for (auto &tile : map)
    {
      if (tile == 20)
      {
        tile = 0;
      }
    }
  }

  if (sprites[i].health.value() / BossValues::initialBossHealth < 0.8 && BossValues::door1 == false)
  {
    BossValues::door1 = true;
    for (auto &tile : map)
    {
      if (tile == 7)
      {
        tile = 0;
        break;
      }
    }
  }

  if (sprites[i].health.value() / BossValues::initialBossHealth < 0.6 && BossValues::door2 == false)
  {
    BossValues::door2 = true;
    for (auto &tile : map)
    {
      if (tile == 7)
      {
        tile = 0;
        break;
      }
    }
  }

  if (sprites[i].health.value() / BossValues::initialBossHealth < 0.4 && BossValues::door3 == false)
  {
    BossValues::door3 = true;
    for (auto &tile : map)
    {
      if (tile == 7)
      {
        tile = 0;
        break;
      }
    }
  }

  if (sprites[i].health.value() / BossValues::initialBossHealth < 0.2 && BossValues::door4 == false)
  {
    BossValues::door4 = true;
    for (auto &tile : map)
    {
      if (tile == 7)
      {
        tile = 0;
        break;
      }
    }
  }

  float deltaX = player.pos.x - sprites[i].x;
  float deltaY = player.pos.y - sprites[i].y;

  float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
  deltaX /= distance;
  deltaY /= distance;

  if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - BossValues::bigAttackTimer).count() > BossValues::bigAttackTime)
  {
    BossValues::bigAttackTimer = std::chrono::high_resolution_clock::now();
    sprites[i].enemyLastBulletTime = std::chrono::high_resolution_clock::now();

    float deltaX = player.pos.x - sprites[i].x;
    float deltaY = player.pos.y - sprites[i].y;

    float angle = atan2(deltaY, deltaX) * (180 / M_PI);

    Sprite bullet;
    bullet.active = true;
    bullet.type = EnemyBullet;
    bullet.x = sprites[i].x;
    bullet.y = sprites[i].y;
    bullet.scaleX = 0.75;
    bullet.scaleY = 0.75;
    bullet.z = 7;
    bullet.direction = angle;
    sprites.emplace_back(bullet);
    for (int i = 0; i < 360; i += 8)
    {
      bullet.direction = angle + i;
      sprites.emplace_back(bullet);
    }
    Mix_PlayChannel(-1, sounds.at(1), 0);
  }

  if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sprites[i].enemyLastBulletTime.value()).count() > BossValues::shootingCooldown)
  {
    sprites[i].enemyLastBulletTime = std::chrono::high_resolution_clock::now();

    std::mt19937 gen(rd());

    std::uniform_int_distribution<int> dist(0, 1);

    float randomNumber = dist(gen);

    float deltaX = player.pos.x - sprites[i].x;
    float deltaY = player.pos.y - sprites[i].y;

    float angle = atan2(deltaY, deltaX) * (180 / M_PI);

    Sprite bullet;
    bullet.active = true;
    bullet.type = EnemyBullet;
    bullet.x = sprites[i].x;
    bullet.y = sprites[i].y;
    bullet.scaleX = 0.75;
    bullet.scaleY = 0.75;
    bullet.z = 7;
    if (randomNumber == 0)
    {
      bullet.direction = angle;
      sprites.emplace_back(bullet);

      bullet.direction = angle + 10;
      sprites.emplace_back(bullet);

      bullet.direction = angle - 10;
      sprites.emplace_back(bullet);

      bullet.direction = angle + 20;
      sprites.emplace_back(bullet);

      bullet.direction = angle - 20;
      sprites.emplace_back(bullet);
    }
    else
    {

      bullet.direction = angle + 5;
      sprites.emplace_back(bullet);

      bullet.direction = angle - 5;
      sprites.emplace_back(bullet);

      bullet.direction = angle + 15;
      sprites.emplace_back(bullet);

      bullet.direction = angle - 15;
      sprites.emplace_back(bullet);

      bullet.direction = angle + 25;
      sprites.emplace_back(bullet);

      bullet.direction = angle - 25;
      sprites.emplace_back(bullet);
    }
    Mix_PlayChannel(-1, sounds.at(1), 0);
  }

  if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - BossValues::strafingTimer).count() > BossValues::strafingTime)
  {
    BossValues::strafeDir = BossValues::strafeDir == 0 ? 1 : 0;
    BossValues::strafingTimer = std::chrono::high_resolution_clock::now();
    BossValues::strafingTime = BossValues::generateStrafingTime();
  }

  if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - BossValues::chargeTimer).count() > BossValues::chargeTime)
  {
    BossValues::chargeDurationTimer = std::chrono::high_resolution_clock::now();
    BossValues::chargeTimer = std::chrono::high_resolution_clock::now();
    BossValues::chargeTime = BossValues::generateChargeTime();
  }

  if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - BossValues::chargeDurationTimer).count() < BossValues::chargeDuration)
  {
    if (distance < 15 && std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sprites[i].enemyLastMeleeTime.value()).count() > enemyMeleeCooldown)
    {
      sprites[i].enemyLastMeleeTime = std::chrono::high_resolution_clock::now();
      health -= 25;
    }

    float chargeSpeed = 750;
    float newX = sprites[i].x + deltaX * chargeSpeed * deltaTime;
    float newY = sprites[i].y + deltaY * chargeSpeed * deltaTime;

    int cellIndexX = floor(newX / cellWidth);
    int cellIndexY = floor(sprites[i].y / cellWidth);
    int mapCellIndexX = getCell(cellIndexX, cellIndexY);

    if (map[mapCellIndexX] == 0)
    {
      sprites[i].x = newX;
    }

    cellIndexX = floor(sprites[i].x / cellWidth);
    cellIndexY = floor(newY / cellWidth);
    int mapCellIndexY = getCell(cellIndexX, cellIndexY);

    if (map[mapCellIndexY] == 0)
    {
      sprites[i].y = newY;
    }
  }

  float enemySpeed = 40;
  float newX;
  float newY;
  if (BossValues::strafeDir == 0)
  {
    newX = sprites[i].x - deltaY * enemySpeed * deltaTime;
    newY = sprites[i].y + deltaX * enemySpeed * deltaTime;
  }
  else
  {
    newX = sprites[i].x + deltaY * enemySpeed * deltaTime;
    newY = sprites[i].y - deltaX * enemySpeed * deltaTime;
  }

  int cellIndexX = floor(newX / cellWidth);
  int cellIndexY = floor(sprites[i].y / cellWidth);
  int mapCellIndexX = getCell(cellIndexX, cellIndexY);

  if (map[mapCellIndexX] == 0)
  {
    sprites[i].x = newX;
  }
  else if (BossValues::chargeTime > 100)
  {
    BossValues::chargeTime = 100;
  }

  cellIndexX = floor(sprites[i].x / cellWidth);
  cellIndexY = floor(newY / cellWidth);
  int mapCellIndexY = getCell(cellIndexX, cellIndexY);

  if (map[mapCellIndexY] == 0)
  {
    sprites[i].y = newY;
  }
  else if (BossValues::chargeTime > 100)
  {
    BossValues::chargeTime = 100;
  }
}

void Game::handleEnemyBullet(int i)
{
  float bulletSpeed = 300;
  float dx = bulletSpeed * cos(degToRad(sprites[i].direction.value())) * deltaTime;
  float dy = bulletSpeed * sin(degToRad(sprites[i].direction.value())) * deltaTime;
  sprites[i].x += dx;
  sprites[i].y += dy;
  float deltaX = player.pos.x - sprites[i].x;
  float deltaY = player.pos.y - sprites[i].y;
  float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
  if (distance < 10)
  {
    health -= 1;
    sprites[i].active = false;
  }

  int cellIndexX = floor(sprites[i].x / cellWidth);
  int cellIndexY = floor(sprites[i].y / cellWidth);

  int mapCellIndex = getCell(cellIndexX, cellIndexY);

  if (map[mapCellIndex] != 0)
  {
    sprites[i].active = false;
  }
}

void Game::handleBullet(int i)
{

  float bulletSpeed = 300;
  float dx = bulletSpeed * cos(degToRad(sprites[i].direction.value())) * deltaTime;
  float dy = bulletSpeed * sin(degToRad(sprites[i].direction.value())) * deltaTime;
  sprites[i].x += dx;
  sprites[i].y += dy;
  for (auto &sprite : sprites)
  {

    if ((sprite.type != Enemy && sprite.type != ShooterEnemy && sprite.type != HammerEnemy && sprite.type != DroneEnemy && sprite.type != Swat) || sprite.active == false)
      continue;

    float deltaX = sprite.x - sprites[i].x;
    float deltaY = sprite.y - sprites[i].y;
    float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
    if (distance < 10)
    {
      sprites[i].active = false;
      sprite.health = sprite.health.value() - gunDamage;
      break;
    }
  }

  int cellIndexX = floor(sprites[i].x / cellWidth);
  int cellIndexY = floor(sprites[i].y / cellWidth);

  int mapCellIndex = getCell(cellIndexX, cellIndexY);

  if (map[mapCellIndex] != 0)
  {
    sprites[i].active = false;
  }
}

void Game::handleEnemyMovement(int i)
{
  if (!sprites[i].soundChannel.has_value())
  {
    sprites[i].soundChannel = Mix_PlayChannel(-1, sounds.at(3), 0);
  }
  if (!Mix_Playing(sprites[i].soundChannel.value()))
  {
    sprites[i].soundChannel = Mix_PlayChannel(-1, sounds.at(3), 0);
  }
  if (sprites[i].health <= 0)
  {
    if (sprites[i].type == Enemy)
    {
      levelMoney += 1;
    }
    else if (sprites[i].type == ShooterEnemy || sprites[i].type == HammerEnemy)
    {
      levelMoney += 2;
    }
    else if (sprites[i].type == DroneEnemy)
    {
      levelMoney += 1;
    }
    sprites[i].active = false;
  }

  float deltaX = player.pos.x - sprites[i].x;
  float deltaY = player.pos.y - sprites[i].y;

  float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
  if (distance < 10)
  {
    if (sprites[i].type == DroneEnemy)
    {
      sprites[i].active = false;
      health -= 5;
    }
    else if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sprites[i].enemyLastMeleeTime.value()).count() > enemyMeleeCooldown && sprites[i].type != HammerEnemy)
    {
      sprites[i].enemyLastMeleeTime = std::chrono::high_resolution_clock::now();
      health -= 5;
    }
    else if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sprites[i].enemyLastMeleeTime.value()).count() > hammerEnemyMeleeCooldown && sprites[i].type == HammerEnemy)
    {
      sprites[i].enemyLastMeleeTime = std::chrono::high_resolution_clock::now();
      health -= 25;
    }
  }

  if (distance > 0)
  {
    deltaX /= distance;
    deltaY /= distance;

    float enemySpeed = 35;
    if (sprites[i].type == HammerEnemy)
    {
      enemySpeed = 20;
    }
    if (sprites[i].type == DroneEnemy)
    {
      enemySpeed = 120;
    }
    float newX = sprites[i].x + deltaX * enemySpeed * deltaTime;
    float newY = sprites[i].y + deltaY * enemySpeed * deltaTime;

    int cellIndexX = floor(newX / cellWidth);
    int cellIndexY = floor(sprites[i].y / cellWidth);
    int mapCellIndexX = getCell(cellIndexX, cellIndexY);

    if (map[mapCellIndexX] == 0)
    {
      sprites[i].x = newX;
    }

    cellIndexX = floor(sprites[i].x / cellWidth);
    cellIndexY = floor(newY / cellWidth);
    int mapCellIndexY = getCell(cellIndexX, cellIndexY);

    if (map[mapCellIndexY] == 0)
    {
      sprites[i].y = newY;
    }
  }
}

void Game::handleShooterEnemy(int i)
{
  sprites[i].enemyLastBulletTime = std::chrono::high_resolution_clock::now();

  float deltaX = player.pos.x - sprites[i].x;
  float deltaY = player.pos.y - sprites[i].y;

  float angle = atan2(deltaY, deltaX) * (180 / M_PI);

  Sprite bullet;
  bullet.active = true;
  bullet.type = EnemyBullet;
  bullet.x = sprites[i].x;
  bullet.y = sprites[i].y;
  bullet.scaleX = 0.5;
  bullet.scaleY = 0.5;
  bullet.z = 7;
  bullet.direction = angle;
  sprites.emplace_back(bullet);
  Mix_PlayChannel(-1, sounds.at(1), 0);
}

void Game::shootBullet()
{
  if (gunType == Pistol && std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastBulletTime).count() > pistolShootingCooldown && spacePressed == false)
  {
    spacePressed = true;
    lastBulletTime = std::chrono::high_resolution_clock::now();

    Sprite bullet;
    bullet.active = true;
    bullet.type = Bullet;
    bullet.x = player.pos.x;
    bullet.y = player.pos.y;
    bullet.scaleX = 0.5;
    bullet.scaleY = 0.5;
    bullet.z = 7;
    bullet.direction = player.angle;
    sprites.emplace_back(bullet);

    Mix_PlayChannel(-1, sounds.at(1), 0);
  }
  else if (gunType == Shotgun && std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastBulletTime).count() > shotgunShootingCooldown && spacePressed == false)
  {
    spacePressed = true;
    lastBulletTime = std::chrono::high_resolution_clock::now();

    Sprite bullet;
    bullet.active = true;
    bullet.type = Bullet;
    bullet.x = player.pos.x;
    bullet.y = player.pos.y;
    bullet.scaleX = 0.5;
    bullet.scaleY = 0.5;
    bullet.z = 7;
    bullet.direction = player.angle;
    sprites.emplace_back(bullet);

    bullet.direction = player.angle + 10;
    sprites.emplace_back(bullet);

    bullet.direction = player.angle - 10;
    sprites.emplace_back(bullet);

    if (playerData.shotgunUpgraded)
    {
      bullet.direction = player.angle + 5;
      sprites.emplace_back(bullet);

      bullet.direction = player.angle - 5;
      sprites.emplace_back(bullet);
    }

    Mix_PlayChannel(-1, sounds.at(1), 0);
  }
  else if (gunType == Minigun && std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastBulletTime).count() > minigunShootingCooldown)
  {
    lastBulletTime = std::chrono::high_resolution_clock::now();

    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> dist(-2, 2);

    float randomNum = dist(gen);

    Sprite bullet;
    bullet.active = true;
    bullet.type = Bullet;
    bullet.x = player.pos.x;
    bullet.y = player.pos.y;
    bullet.scaleX = 0.25;
    bullet.scaleY = 0.25;
    bullet.z = 7;
    bullet.direction = player.angle + randomNum;
    sprites.emplace_back(bullet);

    Mix_PlayChannel(-1, sounds.at(1), 0);
  }
}

void Game::handleInput()
{
  const Uint8 *keystate = SDL_GetKeyboardState(NULL);
  if (keystate[SDL_SCANCODE_SPACE])
  {
    shootBullet();
  }
  else
  {
    spacePressed = false;
  }

  if (keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_Q] || keystate[SDL_SCANCODE_E] || keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_RIGHT])
  {
    if (playerStepChannel == -1)
    {
      playerStepChannel = Mix_PlayChannel(-1, sounds.at(3), 0);
    }
    if (!Mix_Playing(playerStepChannel))
    {
      playerStepChannel = Mix_PlayChannel(-1, sounds.at(3), 0);
    }
  }

  if (keystate[SDL_SCANCODE_LSHIFT])
  {
    rotateSpeed = rotateSpeedSlow;
  }
  else
  {
    rotateSpeed = rotateSpeedFast;
  }

  if (keystate[SDL_SCANCODE_W])
  {
    int cellIndexX = floor(((player.pos.x + (moveSpeed * cos(degToRad(player.angle)) * deltaTime)) * 1.0) / cellWidth);
    int cellIndexY = floor(((player.pos.y + (moveSpeed * sin(degToRad(player.angle)) * deltaTime)) * 1.0) / cellWidth);

    int mapCellIndex = getCell(cellIndexX, cellIndexY);

    if (map[mapCellIndex] == 0)
    {
      player.pos.x += moveSpeed * cos(degToRad(player.angle)) * deltaTime;
      player.pos.y += moveSpeed * sin(degToRad(player.angle)) * deltaTime;
    }
  }

  if (keystate[SDL_SCANCODE_S])
  {
    int cellIndexX = floor(((player.pos.x - (moveSpeed * cos(degToRad(player.angle)) * 1.1 * deltaTime))) / cellWidth);
    int cellIndexY = floor(((player.pos.y - (moveSpeed * sin(degToRad(player.angle)) * 1.1 * deltaTime))) / cellWidth);
    int mapCellIndex = getCell(cellIndexX, cellIndexY);

    if (map[mapCellIndex] == 0)
    {
      player.pos.x -= moveSpeed * cos(degToRad(player.angle)) * deltaTime;
      player.pos.y -= moveSpeed * sin(degToRad(player.angle)) * deltaTime;
    }
  }

  if (keystate[SDL_SCANCODE_A])
  {
    player.angle -= rotateSpeed * deltaTime;
  }
  if (keystate[SDL_SCANCODE_D])
  {
    player.angle += rotateSpeed * deltaTime;
  }

  if (keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_Q])
  {
    int cellIndexX = floor(((player.pos.x + ((moveSpeed / 1.4) * cos(degToRad(player.angle - 90)) * deltaTime)) * 1.0) / cellWidth);
    int cellIndexY = floor(((player.pos.y + ((moveSpeed / 1.4) * sin(degToRad(player.angle - 90)) * deltaTime)) * 1.0) / cellWidth);

    int mapCellIndex = getCell(cellIndexX, cellIndexY);

    if (map[mapCellIndex] == 0)
    {
      player.pos.x += (moveSpeed / 1.5) * cos(degToRad(player.angle - 90)) * deltaTime;
      player.pos.y += (moveSpeed / 1.5) * sin(degToRad(player.angle - 90)) * deltaTime;
    }
  }
  if (keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_E])
  {
    int cellIndexX = floor(((player.pos.x - ((moveSpeed / 1.4) * cos(degToRad(player.angle - 90)) * 1.1 * deltaTime))) / cellWidth);
    int cellIndexY = floor(((player.pos.y - ((moveSpeed / 1.4) * sin(degToRad(player.angle - 90)) * 1.1 * deltaTime))) / cellWidth);
    int mapCellIndex = getCell(cellIndexX, cellIndexY);

    if (map[mapCellIndex] == 0)
    {
      player.pos.x -= (moveSpeed / 1.5) * cos(degToRad(player.angle - 90)) * deltaTime;
      player.pos.y -= (moveSpeed / 1.5) * sin(degToRad(player.angle - 90)) * deltaTime;
    }
  }

  if (keystate[SDL_SCANCODE_1])
  {
    gunType = Pistol;
    gunDamage = 5;
  }
  if (keystate[SDL_SCANCODE_2] && playerData.shotgunUnlocked)
  {
    gunType = Shotgun;
    gunDamage = 5;
  }
  if (keystate[SDL_SCANCODE_3] && playerData.minigunUnlocked)
  {
    gunType = Minigun;
    gunDamage = 1 + playerData.minigunUpgraded;
  }

  if (keystate[SDL_SCANCODE_F])
  {

    int cellIndexX = floor(((player.pos.x + (moveSpeed * cos(degToRad(player.angle)) * 4 * deltaTime))) / cellWidth);
    int cellIndexY = floor(((player.pos.y + (moveSpeed * sin(degToRad(player.angle)) * 4 * deltaTime))) / cellWidth);

    int mapCellIndex = getCell(cellIndexX, cellIndexY);

    if (map[mapCellIndex] == 5)
    {
      map[mapCellIndex] = 0;
    }
    if ((map[mapCellIndex] == 9 || map[mapCellIndex] == 12) && bombCount > 0)
    {
      Mix_PlayChannel(-1, sounds.at(2), 0);
      map[mapCellIndex] = 0;
      bombCount -= 1;
    }
    if (map[mapCellIndex] == 7 && keyCount > 0)
    {
      map[mapCellIndex] = 0;
      keyCount -= 1;
    }
    if (map[mapCellIndex] == 17)
    {
      if (level > playerData.highestLevelBeaten)
      {
        playerData.highestLevelBeaten = level;
      }
      level = 0;
      playerData.money += levelMoney;
      serializePlayer("save.dat");
    }
  }

  for (int x = 0; x < mapX; x++)
  {
    for (int y = 0; y < mapY; y++)
    {
      if (mapFloors[getCell(x, y)] == 19)
      {
        int playerCellIndexX = floor(player.pos.x / cellWidth);
        int playerCellIndexY = floor(player.pos.y / cellWidth);
        if (x == playerCellIndexX && y == playerCellIndexY)
        {
          sprites.clear();
          map.clear();
          mapCeiling.clear();
          mapFloors.clear();
          deserialize("map11.dat");
          deserializeSprites("sprites11.dat");
          player = {{80.0f, 80.0f}, 0.0f, 60};
          health = 100;
        }
      }
    }
  }
}

void Game::displayMainMenu(SDL_Renderer *renderer, TTF_Font *font,
                           SDL_Texture *background, SDL_Texture *titleText, SDL_Rect titleRect)
{
  Button level1(renderer, 57, 200, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Level 1", font, 5, {0, 0, 0, 255});
  Button level2(renderer, 247, 200, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Level 2", font, 5, {0, 0, 0, 255});
  Button level3(renderer, 437, 200, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Level 3", font, 5, {0, 0, 0, 255});
  Button level4(renderer, 627, 200, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Level 4", font, 5, {0, 0, 0, 255});
  Button level5(renderer, 817, 200, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "level 5", font, 5, {0, 0, 0, 255});
  Button level6(renderer, 57, 300, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Level 6", font, 5, {0, 0, 0, 255});
  Button level7(renderer, 247, 300, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Level 7", font, 5, {0, 0, 0, 255});
  Button level8(renderer, 437, 300, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Level 8", font, 5, {0, 0, 0, 255});
  Button level9(renderer, 627, 300, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Level 9", font, 5, {0, 0, 0, 255});
  Button level10(renderer, 817, 300, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "level 10", font, 5, {0, 0, 0, 255});
  Button shop(renderer, 387 + 250, 400, 250, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Shop", font, 5, {0, 0, 0, 255});
  Button achievements(renderer, 387 - 250, 400, 250, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Achievements", font, 5, {0, 0, 0, 255});

  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    if (level1.handleEvent(event))
    {
      sprites.clear();
      map.clear();
      mapCeiling.clear();
      mapFloors.clear();
      deserialize("map.dat");
      deserializeSprites("sprites.dat");
      player = {{80.0f, 80.0f}, 0.0f, 60};
      levelMoney = 0;
      health = 100;
      bombCount = 0;
      keyCount = 0;
      level = 1;
      serializePlayer("save.dat");
    }
    if (level2.handleEvent(event))
    {
      sprites.clear();
      map.clear();
      mapCeiling.clear();
      mapFloors.clear();
      deserialize("map2.dat");
      deserializeSprites("sprites2.dat");
      player = {{80.0f, 80.0f}, 0.0f, 60};
      levelMoney = 0;
      health = 100;
      bombCount = 0;
      keyCount = 0;
      level = 2;
      serializePlayer("save.dat");
    }
    if (level3.handleEvent(event))
    {
      sprites.clear();
      map.clear();
      mapCeiling.clear();
      mapFloors.clear();
      deserialize("map3.dat");
      deserializeSprites("sprites3.dat");
      player = {{80.0f, 80.0f}, 0.0f, 60};
      levelMoney = 0;
      health = 100;
      bombCount = 0;
      keyCount = 0;
      level = 3;
      serializePlayer("save.dat");
    }
    if (level4.handleEvent(event))
    {
      sprites.clear();
      map.clear();
      mapCeiling.clear();
      mapFloors.clear();
      deserialize("map4.dat");
      deserializeSprites("sprites4.dat");
      player = {{80.0f, 80.0f}, 0.0f, 60};
      levelMoney = 0;
      health = 100;
      bombCount = 0;
      keyCount = 0;
      level = 4;
      serializePlayer("save.dat");
    }
    if (level5.handleEvent(event))
    {
      sprites.clear();
      map.clear();
      mapCeiling.clear();
      mapFloors.clear();
      deserialize("map5.dat");
      deserializeSprites("sprites5.dat");
      player = {{80.0f, 80.0f}, 0.0f, 60};
      levelMoney = 0;
      health = 100;
      bombCount = 0;
      keyCount = 0;
      level = 5;
      serializePlayer("save.dat");
    }
    if (level6.handleEvent(event))
    {
      sprites.clear();
      map.clear();
      mapCeiling.clear();
      mapFloors.clear();
      deserialize("map6.dat");
      deserializeSprites("sprites6.dat");
      player = {{80.0f, 80.0f}, 0.0f, 60};
      levelMoney = 0;
      health = 100;
      bombCount = 0;
      keyCount = 0;
      level = 6;
      serializePlayer("save.dat");
    }
    if (level7.handleEvent(event))
    {
      sprites.clear();
      map.clear();
      mapCeiling.clear();
      mapFloors.clear();
      deserialize("map7.dat");
      deserializeSprites("sprites7.dat");
      player = {{80.0f, 80.0f}, 0.0f, 60};
      levelMoney = 0;
      health = 100;
      bombCount = 0;
      keyCount = 0;
      level = 7;
      serializePlayer("save.dat");
    }
    if (level8.handleEvent(event))
    {
      sprites.clear();
      map.clear();
      mapCeiling.clear();
      mapFloors.clear();
      deserialize("map8.dat");
      deserializeSprites("sprites8.dat");
      player = {{80.0f, 80.0f}, 0.0f, 60};
      levelMoney = 0;
      health = 100;
      bombCount = 0;
      keyCount = 0;
      level = 8;
      serializePlayer("save.dat");
    }
    if (level9.handleEvent(event))
    {
      sprites.clear();
      map.clear();
      mapCeiling.clear();
      mapFloors.clear();
      deserialize("map9.dat");
      deserializeSprites("sprites9.dat");
      player = {{80.0f, 80.0f}, 0.0f, 60};
      levelMoney = 0;
      health = 100;
      bombCount = 0;
      keyCount = 0;
      level = 9;
      serializePlayer("save.dat");
    }
    if (level10.handleEvent(event))
    {
      sprites.clear();
      map.clear();
      mapCeiling.clear();
      mapFloors.clear();
      deserialize("map10.dat");
      deserializeSprites("sprites10.dat");
      player = {{80.0f, 80.0f}, 0.0f, 60};
      levelMoney = 0;
      health = 100;
      bombCount = 0;
      keyCount = 0;
      level = 10;
      serializePlayer("save.dat");
    }
    if (shop.handleEvent(event))
    {
      level = -1;
    }
    if (achievements.handleEvent(event))
    {
      Achievements::update();
      level = -2;
    }
  }
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  SDL_RenderCopy(renderer, background, NULL, NULL);

  level1.render();
  level2.render();
  level3.render();
  level4.render();
  level5.render();
  level6.render();
  level7.render();
  level8.render();
  level9.render();
  level10.render();
  shop.render();
  achievements.render();

  SDL_RenderCopy(renderer, titleText, NULL, &titleRect);

  SDL_RenderPresent(renderer);

  SDL_Delay(16);
}

void Game::displayShop(SDL_Renderer *renderer, TTF_Font *font, SDL_Texture *pistol, SDL_Texture *shotgun,
                       SDL_Texture *minigun, SDL_Color coinTextColor)
{
  Button back(renderer, 57, 400, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Back", font, 5, {0, 0, 0, 255});

  std::string pistolText = playerData.pistolUpgraded ? "Max" : "Upgrade Pistol - Cost: 100";
  Button pistolBtn(renderer, 50, 300, 275, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, pistolText, font, 5, {0, 0, 0, 255}, 0.7);
  std::string shotgunText = playerData.shotgunUpgraded ? "Max" : playerData.shotgunUnlocked ? "Upgrade Shotgun - Cost: 500"
                                                                                            : "Buy Shotgun - Cost: 250";
  Button shotgunBtn(renderer, 375, 300, 275, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, shotgunText, font, 5, {0, 0, 0, 255}, 0.7);

  std::string minigunText = playerData.minigunUpgraded ? "Max" : playerData.minigunUnlocked ? "Upgrade Minigun - Cost: 1000"
                                                                                            : "Buy Minigun - Cost: 500";
  Button minigunBtn(renderer, 700, 300, 275, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, minigunText, font, 5, {0, 0, 0, 255}, 0.7);

  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    if (back.handleEvent(event))
    {
      level = 0;
    }
    if (pistolBtn.handleEvent(event))
    {
      if (playerData.money >= 100 && playerData.pistolUpgraded == false)
      {
        playerData.pistolUpgraded = true;
        pistolShootingCooldown -= 100;
        playerData.money -= 100;
        serializePlayer("save.dat");
      }
    }
    if (shotgunBtn.handleEvent(event))
    {
      if (playerData.shotgunUnlocked == false)
      {
        if (playerData.money >= 250)
        {
          playerData.shotgunUnlocked = true;
          playerData.money -= 250;
          serializePlayer("save.dat");
        }
      }
      else
      {
        if (playerData.money >= 500 && playerData.shotgunUpgraded == false)
        {
          playerData.shotgunUpgraded = true;
          playerData.money -= 500;
          serializePlayer("save.dat");
        }
      }
    }

    if (minigunBtn.handleEvent(event))
    {
      if (playerData.minigunUnlocked == false)
      {
        if (playerData.money >= 500)
        {
          playerData.minigunUnlocked = true;
          playerData.money -= 500;
          serializePlayer("save.dat");
        }
      }
      else
      {
        if (playerData.money >= 1000 && playerData.minigunUpgraded == false)
        {
          playerData.minigunUpgraded = true;
          minigunShootingCooldown -= 5;
          playerData.money -= 1000;
          serializePlayer("save.dat");
        }
      }
    }
  }

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  std::string text = "Money: " + std::to_string(playerData.money);
  SDL_Surface *textSurface = TTF_RenderText_Solid(font, text.c_str(), coinTextColor);
  if (!textSurface)
  {
    printf("Error rendering text: %s\n", TTF_GetError());
    exit(EXIT_FAILURE);
  }

  SDL_Texture *coinText = SDL_CreateTextureFromSurface(renderer, textSurface);

  SDL_Rect textRect = {20, 20, textSurface->w, textSurface->h};
  SDL_RenderCopy(renderer, coinText, NULL, &textRect);

  SDL_Rect pistolRect = {50, 145, 200, 150};
  SDL_RenderCopy(renderer, pistol, NULL, &pistolRect);

  SDL_Rect shotgunRect = {375, 165, 200, 75};
  SDL_RenderCopy(renderer, shotgun, NULL, &shotgunRect);

  SDL_Rect minigunRect = {700, 165, 200, 100};
  SDL_RenderCopy(renderer, minigun, NULL, &minigunRect);

  SDL_FreeSurface(textSurface);
  SDL_DestroyTexture(coinText);

  back.render();
  pistolBtn.render();
  shotgunBtn.render();
  minigunBtn.render();

  SDL_RenderPresent(renderer);

  SDL_Delay(16);
}

void Game::displayAchievements(SDL_Renderer *renderer, TTF_Font *font)
{
  Button back(renderer, 57, 400, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Back", font, 5, {0, 0, 0, 255});
  Text beatenAllLevels(renderer, font, 50, 20, "Beat All Levels", 0.9);
  SDL_Rect beatenAllLevelsRect = {20, 20, 20, 20};

  Text gotAllWeaponsUpgraded(renderer, font, 50, 60, "Upgrade All Weapons", 0.9);
  SDL_Rect gotAllWeaponsUpgradedRect = {20, 60, 20, 20};

  Text hundretPercenter(renderer, font, 50, 100, "Earn All Achievements", 0.9);
  SDL_Rect hundretPercenterRect = {20, 100, 20, 20};

  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    if (back.handleEvent(event))
    {
      level = 0;
    }
  }

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  back.render();

  if (Achievements::beatenAllLevels == true)
  {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  }
  else
  {
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
  }
  SDL_RenderFillRect(renderer, &beatenAllLevelsRect);
  beatenAllLevels.render();

  if (Achievements::gotAllWeaponsUpgraded == true)
  {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  }
  else
  {
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
  }
  SDL_RenderFillRect(renderer, &gotAllWeaponsUpgradedRect);
  gotAllWeaponsUpgraded.render();

  if (Achievements::hundredPercenter == true)
  {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  }
  else
  {
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
  }
  SDL_RenderFillRect(renderer, &hundretPercenterRect);
  hundretPercenter.render();

  SDL_RenderPresent(renderer);

  SDL_Delay(16);
}