#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <vector>
#include <fstream>
#include <optional>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const float moveSpeed = 100.f;
const float rotateSpeed = 100.f;

bool gameRunning = true;

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
    Coin
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
};

std::vector<Sprite> sprites;

auto currentTime = std::chrono::high_resolution_clock::now();
auto lastTime = std::chrono::high_resolution_clock::now();

float deltaTime;

const float rayStep = 0.25;

std::vector<std::string> textureFilepaths = {
    "./textures/texture-1.png",
    "./textures/texture-2.png",
    "./textures/texture-3.png",
    "./textures/texture-4.png",
    "./textures/texture-5.png",
    "./textures/texture-6.png",
    "./textures/safeDoor.png",
    "./textures/brickWall.png",
    "./textures/crackedBrickWall.png",
    "./textures/tiledFloor.png",
    "./textures/enemy.png",
    "./textures/bomb.png",
    "./textures/bullet.png",
    "./textures/key.png",
    "./textures/coin.png",
};

struct Texture
{
    int width, height, channels;
    unsigned char *data;
};

std::vector<Texture> loadedTextures;

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

float distances[241];

int mapX;
int mapY;
int cellWidth = 64;
int maxDepth;

std::vector<int> map;
std::vector<int> mapFloors;
std::vector<int> mapCeiling;

int health = 30;
int money = 0;

int bombCount = 0;
int keyCount = 0;
const int shootingCooldown = 500;
auto lastBulletTime = std::chrono::high_resolution_clock::now();
int gunDamage = 5;

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
        std::cout << "Deserializing map with count: " << count << "\n";
        map.resize(count);
        file.read(reinterpret_cast<char *>(map.data()), sizeof(int) * count);

        count;
        file.read(reinterpret_cast<char *>(&count), sizeof(count));
        std::cout << "Deserializing mapFloors with count: " << count << "\n";
        mapFloors.resize(count);
        file.read(reinterpret_cast<char *>(mapFloors.data()), sizeof(int) * count);

        count;
        file.read(reinterpret_cast<char *>(&count), sizeof(count));
        std::cout << "Deserializing mapCeiling with count: " << count << "\n";
        mapCeiling.resize(count);
        file.read(reinterpret_cast<char *>(mapCeiling.data()), sizeof(int) * count);
        file.close();
    }
    else
    {
        std::cerr << "Error opening file for reading.\n";
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
            Sprite sprite;
            int type;
            file.read(reinterpret_cast<char *>(&type), sizeof(int));
            sprite.type = static_cast<SpriteType>(type - 1);
            file.read(reinterpret_cast<char *>(&sprite.x), sizeof(float));
            file.read(reinterpret_cast<char *>(&sprite.y), sizeof(float));
            file.read(reinterpret_cast<char *>(&sprite.z), sizeof(float));
            if (sprite.type == Enemy || sprite.type == ShooterEnemy)
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
            if (sprite.type == ShooterEnemy && hasHealth == false)
            {
                sprite.health = 10;
            }
            if (sprite.type == ShooterEnemy)
            {
                sprite.enemyLastBulletTime = std::chrono::high_resolution_clock::now();
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
            std::cout << "Deserialized sprite: "
                      << "Type: " << sprite.type
                      << ", X: " << sprite.x
                      << ", Y: " << sprite.y
                      << ", Z: " << sprite.z
                      << ", Health: " << (sprite.health.has_value() ? *sprite.health : 0.0f)
                      << ", Direction: " << (sprite.direction.has_value() ? *sprite.direction : 0.0f)
                      << std::endl;
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
        std::cerr << "Invalid hitType: " << hitType << std::endl;
        return;
    }

    const Texture &tex = loadedTextures[hitType - 1];
    if (x < 0 || x >= tex.width || y < 0 || y >= tex.height)
    {
        std::cerr << "Coordinates out of bounds: " << x << ", " << y << std::endl;
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
        std::cerr << "Invalid hitType: " << hitType << std::endl;
        return;
    }

    const Texture &tex = loadedTextures[hitType - 1];
    if (x < 0 || x >= tex.width || y < 0 || y >= tex.height)
    {
        std::cerr << "Coordinates out of bounds: " << x << ", " << y << std::endl;
        return;
    }

    int index = (y * tex.width + x) * 4;
    r = tex.data[index];
    g = tex.data[index + 1];
    b = tex.data[index + 2];
    a = tex.data[index + 3];
}

float degToRad(float angle) { return angle * M_PI / 180.0; }

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

void drawMap(SDL_Renderer *renderer)
{
    for (int y = 0; y < mapY; y++)
    {
        for (int x = 0; x < mapX; x++)
        {
            int cell = getCell(x, y);

            if (map[cell] == 1)
            {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            }
            else
            {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            }

            SDL_Rect rect;
            rect.x = x * cellWidth;
            rect.y = y * cellWidth;
            rect.w = cellWidth;
            rect.h = cellWidth;

            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

void raycast(Player *player, SDL_Renderer *renderer)
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
        distances[static_cast<int>(i / rayStep)] = distance;
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

void drawSprites(SDL_Renderer *renderer, Player *player)
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
                sprites[i].active = false;
                money += 5;
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

        if (sprites[i].type == Enemy || sprites[i].type == ShooterEnemy)
        {
            if (sprites[i].health <= 0)
            {
                if (sprites[i].type == Enemy)
                {
                    money += 1;
                }
                else if (sprites[i].type == ShooterEnemy)
                {
                    money += 2;
                }
                sprites[i].active = false;
                continue;
            }

            float deltaX = player->pos.x - sprites[i].x;
            float deltaY = player->pos.y - sprites[i].y;

            float distance = std::sqrt(deltaX * deltaX + deltaY * deltaY);
            if (distance < 10)
            {
                gameRunning = false;
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

        if (sprites[i].type == ShooterEnemy && std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - sprites[i].enemyLastBulletTime.value()).count() > shootingCooldown)
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
                textureIndex = 13;
            }

            if (sprites[i].type == Coin)
            {
                textureIndex = 14;
            }

            if (sprites[i].type == Bomb)
            {
                textureIndex = 11;
            }

            if (sprites[i].type == Bullet || sprites[i].type == EnemyBullet)
            {
                textureIndex = 12;
            }

            if (sprites[i].type == Enemy || sprites[i].type == ShooterEnemy)
            {
                textureIndex = 10;
            }

            for (int x = 0; x < loadedTextures[textureIndex].width; x++)
            {
                float recX = projectedX + ((x * (256 * sprites[i].scaleX)) / distance);

                recX -= ((preCalculatedWidth * loadedTextures[textureIndex].width) / 8);

                if (static_cast<int>(glm::clamp((recX * 240) / 1024, 0.f, 240.f)) >= 0 && static_cast<int>(glm::clamp((recX * 240) / 1024, 0.f, 240.f)) < 241 && distance < distances[static_cast<int>(glm::clamp((recX * 240) / 1024, 0.f, 240.f))])
                {

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

void handleInput(Player *player)
{
    const Uint8 *keystate = SDL_GetKeyboardState(NULL);

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

    if (keystate[SDL_SCANCODE_E])
    {

        int cellIndexX = floor(((player->pos.x + (moveSpeed * cos(degToRad(player->angle)) * 4 * deltaTime))) / cellWidth);
        int cellIndexY = floor(((player->pos.y + (moveSpeed * sin(degToRad(player->angle)) * 4 * deltaTime))) / cellWidth);

        int mapCellIndex = getCell(cellIndexX, cellIndexY);

        if (map[mapCellIndex] == 5)
        {
            map[mapCellIndex] = 0;
        }
        if (map[mapCellIndex] == 9 && bombCount > 0)
        {
            map[mapCellIndex] = 0;
            bombCount -= 1;
        }
        if (map[mapCellIndex] == 7 && keyCount > 0)
        {
            map[mapCellIndex] = 0;
            keyCount -= 1;
        }
    }
}

int main()
{

    loadTextures();
    std::cout << loadedTextures.size();
    deserialize("map.dat");
    deserializeSprites("sprites.dat");

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Psudo 3d Raytracer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 512, SDL_WINDOW_SHOWN);
    if (!window)
    {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Player player = {{80.0f, 80.0f}, 0.0f, 60};

      auto startTime = std::chrono::high_resolution_clock::now();
    lastTime = std::chrono::high_resolution_clock::now();
    while (gameRunning)
    {
        currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - startTime;
        deltaTime = ((std::chrono::duration<float>)(currentTime - lastTime)).count();

        lastTime = currentTime;

        if (health <= 0)
        {
            gameRunning = false;
            continue;
        }

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                gameRunning = false;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN)
            {

                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastBulletTime).count() < shootingCooldown)
                        continue;
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
                }
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
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}