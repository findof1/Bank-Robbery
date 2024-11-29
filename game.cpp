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
      displayMainMenu(renderer, &player, font, background, titleText, titleRect);
      continue;
    }
    else if (level == -1)
    {
      displayShop(renderer, font, pistol, shotgun, minigun, coinTextColor);
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

    handleInput(&player);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    // drawMap(renderer);

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

    raycast(&player, renderer);
    // SDL_RenderDrawPoint(renderer, static_cast<int>(player.pos.x), static_cast<int>(player.pos.y));

    drawSprites(renderer, &player);

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

void Game::raycast(Player *player, SDL_Renderer *renderer)
{
  float rayAngle = FixAngle(player->angle - (player->FOV / 2));

  for (float i = 0; i < player->FOV; i += rayStep)
  {
    float rayX = player->pos.x;
    float rayY = player->pos.y;
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
        distanceHorizontal = sqrt(pow(rayX - player->pos.x, 2) + pow(rayY - player->pos.y, 2));
      }
      if (map[getCell(cellIndexX, cellIndexY - 1)] != 0)
      {
        hitTypeHorizontal = map[getCell(cellIndexX, cellIndexY - 1)];
        depth = maxDepth;
        mappedPosHorizontal = static_cast<int>((rayX - cellIndexX * cellWidth) / 2.0f);
        distanceHorizontal = sqrt(pow(rayX - player->pos.x, 2) + pow(rayY - player->pos.y, 2));
      }
      depth++;
    }

    float horizontalRayX = rayX;
    float horizontalRayY = rayY;
    // vertical

    int mappedPosVertical;
    int hitTypeVertical;

    rayX = player->pos.x;
    rayY = player->pos.y;

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
        distanceVertical = sqrt(pow(rayX - player->pos.x, 2) + pow(rayY - player->pos.y, 2));
      }
      if (map[getCell(cellIndexX - 1, cellIndexY)] != 0)
      {
        hitTypeVertical = map[getCell(cellIndexX - 1, cellIndexY)];
        depth = maxDepth;
        mappedPosVertical = static_cast<int>((rayY - cellIndexY * cellWidth) / 2.0f);
        distanceVertical = sqrt(pow(rayX - player->pos.x, 2) + pow(rayY - player->pos.y, 2));
      }
      depth++;
    }

    /*
SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
SDL_RenderDrawLine(renderer, player->pos.x, player->pos.y, horizontalRayX, horizontalRayY);
SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
SDL_RenderDrawLine(renderer, player->pos.x, player->pos.y, rayX, rayY);
*/
    int mappedPos;
    int hitType;
    // SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    if (distanceVertical < distanceHorizontal)
    {
      // SDL_RenderDrawLine(renderer, player->pos.x, player->pos.y, horizontalRayX, horizontalRayY);
      mappedPos = mappedPosVertical;
      hitType = hitTypeVertical;
      SDL_SetRenderDrawColor(renderer, 0, 155, 0, 255);
    }
    else
    {
      mappedPos = mappedPosHorizontal;
      hitType = hitTypeHorizontal;
      SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
      // SDL_RenderDrawLine(renderer, player->pos.x, player->pos.y, rayX, rayY);
    }

    float distance = std::min(distanceHorizontal, distanceVertical);
    float correctedDistance = distance * cos(degToRad(FixAngle(player->angle - rayAngle)));
    distances.emplace_back(distance);
    SDL_FRect rectangle;
    rectangle.x = i * (1024 / (player->FOV));
    rectangle.h = (64 * 512) / correctedDistance;
    rectangle.y = (512 / 2) - (rectangle.h / 2);
    rectangle.w = (1024 / (player->FOV)) * rayStep;
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
    float rayAngleFix = cos(degToRad(FixAngle(player->angle - rayAngle)));
    float drawX = i * (1024 / (player->FOV));
    float drawWidth = (1024 / (player->FOV)) * rayStep;
    for (int y = rectangle.y + rectangle.h; y < 512; y += drawWidth / 1.5)
    {
      float dy = y - (512 / 2.0);
      float textureX = player->pos.x / 2 + cos(deg) * 126 * 2 * 32 / dy / rayAngleFix;
      float textureY = player->pos.y / 2 - sin(deg) * 126 * 2 * 32 / dy / rayAngleFix;
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
      textureX = player->pos.x / 2 + cos(deg) * 126 * 2 * 32 / dy / rayAngleFix;
      textureY = player->pos.y / 2 - sin(deg) * 126 * 2 * 32 / dy / rayAngleFix;
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

void Game::drawSprites(SDL_Renderer *renderer, Player *player)
{
  std::sort(sprites.begin(), sprites.end(),
            [player](const Sprite &a, const Sprite &b)
            {
              return glm::distance(glm::vec2(a.x, a.y), glm::vec2(player->pos.x, player->pos.y)) > glm::distance(glm::vec2(b.x, b.y), glm::vec2(player->pos.x, player->pos.y));
            });

  for (int i = 0; i < sprites.size(); i++)
  {
    if (sprites[i].active == false)
      continue;

    if (sprites[i].type == Key)
    {
      float distance = sqrt(pow(sprites[i].x - player->pos.x, 2) + pow(sprites[i].y - player->pos.y, 2));
      if (distance < 15)
      {
        sprites[i].active = false;
        keyCount += 1;
      }
    }

    if (sprites[i].type == Coin)
    {
      float distance = sqrt(pow(sprites[i].x - player->pos.x, 2) + pow(sprites[i].y - player->pos.y, 2));
      if (distance < 15)
      {
        Mix_PlayChannel(-1, sounds.at(0), 0);
        sprites[i].active = false;
        levelMoney += 5;
      }
    }

    if (sprites[i].type == Bomb)
    {
      float distance = sqrt(pow(sprites[i].x - player->pos.x, 2) + pow(sprites[i].y - player->pos.y, 2));
      if (distance < 15)
      {
        sprites[i].active = false;
        bombCount += 1;
      }
    }

    if (sprites[i].type == Bullet)
    {
      float bulletSpeed = 300;
      float dx = bulletSpeed * cos(degToRad(sprites[i].direction.value())) * deltaTime;
      float dy = bulletSpeed * sin(degToRad(sprites[i].direction.value())) * deltaTime;
      sprites[i].x += dx;
      sprites[i].y += dy;
      for (auto &sprite : sprites)
      {
        if ((sprite.type != Enemy && sprite.type != ShooterEnemy) || sprite.active == false)
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

      if (sprites[i].active == false)
        continue;
    }

    if (sprites[i].type == EnemyBullet)
    {
      float bulletSpeed = 300;
      float dx = bulletSpeed * cos(degToRad(sprites[i].direction.value())) * deltaTime;
      float dy = bulletSpeed * sin(degToRad(sprites[i].direction.value())) * deltaTime;
      sprites[i].x += dx;
      sprites[i].y += dy;
      float deltaX = player->pos.x - sprites[i].x;
      float deltaY = player->pos.y - sprites[i].y;
      float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
      if (distance < 10)
      {
        health -= 1;
        sprites[i].active = false;
        continue;
      }

      int cellIndexX = floor(sprites[i].x / cellWidth);
      int cellIndexY = floor(sprites[i].y / cellWidth);

      int mapCellIndex = getCell(cellIndexX, cellIndexY);

      if (map[mapCellIndex] != 0)
      {
        sprites[i].active = false;
        continue;
      }
    }

    if ((sprites[i].type == Enemy || sprites[i].type == ShooterEnemy) && sprites[i].move == true)
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
        else if (sprites[i].type == ShooterEnemy)
        {
          levelMoney += 2;
        }
        sprites[i].active = false;
        continue;
      }

      float deltaX = player->pos.x - sprites[i].x;
      float deltaY = player->pos.y - sprites[i].y;

      float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
      if (distance < 10 && std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sprites[i].enemyLastMeleeTime.value()).count() > 1000)
      {
        sprites[i].enemyLastMeleeTime = std::chrono::high_resolution_clock::now();
        health -= 5;
      }

      if (distance > 0)
      {
        deltaX /= distance;
        deltaY /= distance;

        float enemySpeed = 35;
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

    if (sprites[i].type == ShooterEnemy && std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sprites[i].enemyLastBulletTime.value()).count() > enemyShootingCooldown && sprites[i].move == true)
    {
      sprites[i].enemyLastBulletTime = std::chrono::high_resolution_clock::now();

      float deltaX = player->pos.x - sprites[i].x;
      float deltaY = player->pos.y - sprites[i].y;

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

    float spriteX = sprites[i].x - player->pos.x;
    float spriteY = sprites[i].y - player->pos.y;
    float spriteZ = sprites[i].z;

    float angleRad = -degToRad(player->angle);
    float rotatedX = spriteY * cos(angleRad) + spriteX * sin(angleRad);
    float rotatedY = spriteX * cos(angleRad) - spriteY * sin(angleRad);

    if (rotatedY > 0)
    {

      float fovFactor = 512.0f / tan(degToRad(player->FOV / 2));

      float projectedX = (rotatedX * fovFactor / rotatedY) + (1024 / 2);
      float projectedY = (spriteZ * fovFactor / rotatedY) + (512 / 2);

      float distance = sqrt(pow(spriteX, 2) + pow(spriteY, 2));

      float preCalculatedWidth = ((1024 / (player->FOV)) * rayStep + (1024.f / distance)) * 0.45 * sprites[i].scaleX;
      float preCalculatedHeight = ((1024 / (player->FOV)) * rayStep + (1024.f / distance)) * 0.45 * sprites[i].scaleX;

      int textureIndex;

      if (sprites[i].type == Key)
      {
        textureIndex = 20;
      }

      if (sprites[i].type == Coin)
      {
        textureIndex = 21;
      }

      if (sprites[i].type == Bomb)
      {
        textureIndex = 18;
      }

      if (sprites[i].type == Bullet || sprites[i].type == EnemyBullet)
      {
        textureIndex = 19;
      }

      if (sprites[i].type == Enemy || sprites[i].type == ShooterEnemy)
      {
        textureIndex = 17;
      }

      for (int x = 0; x < loadedTextures[textureIndex].width; x++)
      {
        float recX = projectedX + ((x * (256 * sprites[i].scaleX)) / distance);

        recX -= ((preCalculatedWidth * loadedTextures[textureIndex].width) / 8);

        if (static_cast<int>(glm::clamp((recX * (player->FOV / rayStep)) / 1024, 0.f, (player->FOV / rayStep))) - 1 >= 0 && static_cast<int>(glm::clamp((recX * (player->FOV / rayStep)) / 1024, 0.f, (player->FOV / rayStep))) - 1 <= (player->FOV / rayStep) && distance < distances.at(static_cast<int>(glm::clamp((recX * (player->FOV / rayStep)) / 1024, 0.f, (player->FOV / rayStep))) - 1))
        {

          if ((sprites[i].type == Enemy || sprites[i].type == ShooterEnemy) && sprites[i].move == false && recX < 1024)
          {
            float deltaX = player->pos.x - sprites[i].x;
            float deltaY = player->pos.y - sprites[i].y;

            float distancePlayer = std::sqrt(deltaX * deltaX + deltaY * deltaY);
            if (distancePlayer < 1000)
            {
              sprites[i].move = true;
            }
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
}

void Game::shootBullet(const Player &player)
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

void Game::handleInput(Player *player)
{
  SDL_PumpEvents();
  const Uint8 *keystate = SDL_GetKeyboardState(NULL);

  if (keystate[SDL_SCANCODE_SPACE])
  {
    shootBullet(*player);
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
    int cellIndexX = floor(((player->pos.x + (moveSpeed * cos(degToRad(player->angle)) * deltaTime)) * 1.0) / cellWidth);
    int cellIndexY = floor(((player->pos.y + (moveSpeed * sin(degToRad(player->angle)) * deltaTime)) * 1.0) / cellWidth);

    int mapCellIndex = getCell(cellIndexX, cellIndexY);

    if (map[mapCellIndex] == 0)
    {
      player->pos.x += moveSpeed * cos(degToRad(player->angle)) * deltaTime;
      player->pos.y += moveSpeed * sin(degToRad(player->angle)) * deltaTime;
    }
  }
  if (keystate[SDL_SCANCODE_S])
  {
    int cellIndexX = floor(((player->pos.x - (moveSpeed * cos(degToRad(player->angle)) * 1.1 * deltaTime))) / cellWidth);
    int cellIndexY = floor(((player->pos.y - (moveSpeed * sin(degToRad(player->angle)) * 1.1 * deltaTime))) / cellWidth);
    int mapCellIndex = getCell(cellIndexX, cellIndexY);

    if (map[mapCellIndex] == 0)
    {
      player->pos.x -= moveSpeed * cos(degToRad(player->angle)) * deltaTime;
      player->pos.y -= moveSpeed * sin(degToRad(player->angle)) * deltaTime;
    }
  }

  if (keystate[SDL_SCANCODE_A])
  {
    player->angle -= rotateSpeed * deltaTime;
  }
  if (keystate[SDL_SCANCODE_D])
  {
    player->angle += rotateSpeed * deltaTime;
  }

  if (keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_Q])
  {
    int cellIndexX = floor(((player->pos.x + ((moveSpeed / 1.4) * cos(degToRad(player->angle - 90)) * deltaTime)) * 1.0) / cellWidth);
    int cellIndexY = floor(((player->pos.y + ((moveSpeed / 1.4) * sin(degToRad(player->angle - 90)) * deltaTime)) * 1.0) / cellWidth);

    int mapCellIndex = getCell(cellIndexX, cellIndexY);

    if (map[mapCellIndex] == 0)
    {
      player->pos.x += (moveSpeed / 1.5) * cos(degToRad(player->angle - 90)) * deltaTime;
      player->pos.y += (moveSpeed / 1.5) * sin(degToRad(player->angle - 90)) * deltaTime;
    }
  }
  if (keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_E])
  {
    int cellIndexX = floor(((player->pos.x - ((moveSpeed / 1.4) * cos(degToRad(player->angle - 90)) * 1.1 * deltaTime))) / cellWidth);
    int cellIndexY = floor(((player->pos.y - ((moveSpeed / 1.4) * sin(degToRad(player->angle - 90)) * 1.1 * deltaTime))) / cellWidth);
    int mapCellIndex = getCell(cellIndexX, cellIndexY);

    if (map[mapCellIndex] == 0)
    {
      player->pos.x -= (moveSpeed / 1.5) * cos(degToRad(player->angle - 90)) * deltaTime;
      player->pos.y -= (moveSpeed / 1.5) * sin(degToRad(player->angle - 90)) * deltaTime;
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
    gunDamage = 2 + playerData.minigunUpgraded;
  }

  if (keystate[SDL_SCANCODE_F])
  {

    int cellIndexX = floor(((player->pos.x + (moveSpeed * cos(degToRad(player->angle)) * 4 * deltaTime))) / cellWidth);
    int cellIndexY = floor(((player->pos.y + (moveSpeed * sin(degToRad(player->angle)) * 4 * deltaTime))) / cellWidth);

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
      level = 0;
      playerData.money += levelMoney;
    }
  }
}

void Game::displayMainMenu(SDL_Renderer *renderer, Player *player, TTF_Font *font,
                           SDL_Texture *background, SDL_Texture *titleText, SDL_Rect titleRect)
{
  Button level1(renderer, 57, 300, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Level 1", font, 5, {0, 0, 0, 255});
  Button level2(renderer, 247, 300, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Level 2", font, 5, {0, 0, 0, 255});
  Button level3(renderer, 437, 300, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Level 3", font, 5, {0, 0, 0, 255});
  Button level4(renderer, 627, 300, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Level 4", font, 5, {0, 0, 0, 255});
  Button level5(renderer, 817, 300, 150, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Coming Soon", font, 5, {0, 0, 0, 255});
  Button shop(renderer, 387, 400, 250, 75, {200, 200, 200, 255}, {210, 210, 210, 255}, "Shop", font, 5, {0, 0, 0, 255});
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
      player = new Player({{80.0f, 80.0f}, 0.0f, 60});
      levelMoney = 0;
      health = 100;
      bombCount = 0;
      keyCount = 0;
      level = 1;
    }
    if (level2.handleEvent(event))
    {
      sprites.clear();
      map.clear();
      mapCeiling.clear();
      mapFloors.clear();
      deserialize("map2.dat");
      deserializeSprites("sprites2.dat");
      player = new Player({{80.0f, 80.0f}, 0.0f, 60});
      levelMoney = 0;
      health = 100;
      bombCount = 0;
      keyCount = 0;
      level = 2;
    }
    if (level3.handleEvent(event))
    {
      sprites.clear();
      map.clear();
      mapCeiling.clear();
      mapFloors.clear();
      deserialize("map3.dat");
      deserializeSprites("sprites3.dat");
      player = new Player({{80.0f, 80.0f}, 0.0f, 60});
      levelMoney = 0;
      health = 100;
      bombCount = 0;
      keyCount = 0;
      level = 3;
    }
    if (level4.handleEvent(event))
    {
      sprites.clear();
      map.clear();
      mapCeiling.clear();
      mapFloors.clear();
      deserialize("map4.dat");
      deserializeSprites("sprites4.dat");
      player = new Player({{80.0f, 80.0f}, 0.0f, 60});
      levelMoney = 0;
      level = 4;
    }
    if (shop.handleEvent(event))
    {
      deserialize("map4.dat");
      deserializeSprites("sprites4.dat");
      level = -1;
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
  shop.render();

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
        }
      }
      else
      {
        if (playerData.money >= 500 && playerData.shotgunUpgraded == false)
        {
          playerData.shotgunUpgraded = true;
          playerData.money -= 500;
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
        }
      }
      else
      {
        if (playerData.money >= 1000 && playerData.minigunUpgraded == false)
        {
          playerData.minigunUpgraded = true;
          minigunShootingCooldown -= 25;
          playerData.money -= 1000;
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