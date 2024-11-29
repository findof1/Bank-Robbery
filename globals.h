#pragma once
#include "types.h"
#include <vector>
std::vector<Texture> loadedTextures;
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
    "./textures/blueBrickWall.png",
    "./textures/crackedBlueBrickWall.png",
    "./textures/carpet.png",
    "./textures/tiledCeiling.png",
    "./textures/metalPlate.png",
    "./textures/marble.png",
    "./textures/exit.png",
    "./textures/enemy.png",
    "./textures/bomb.png",
    "./textures/bullet.png",
    "./textures/key.png",
    "./textures/coin.png",
};

const float moveSpeed = 100.f;
float rotateSpeed;
float rotateSpeedFast = 180;
float rotateSpeedSlow = 30;

bool gameRunning = true;

std::random_device rd;

std::vector<float> distances;

int mapX;
int mapY;
int cellWidth = 64;
int maxDepth;

std::vector<int> map;
std::vector<int> mapFloors;
std::vector<int> mapCeiling;

int health = 100;
int levelMoney = 0;

int level = 0;
int bombCount = 0;
int keyCount = 0;
int enemyShootingCooldown = 500;
int pistolShootingCooldown = 500;
int shotgunShootingCooldown = 1000;
int minigunShootingCooldown = 75;
std::chrono::_V2::system_clock::time_point lastBulletTime = std::chrono::high_resolution_clock::now();
int gunDamage = 5;
int gunType = Pistol;
bool spacePressed = false;

PlayerData playerData;

std::vector<Sprite> sprites;
std::vector<Mix_Chunk *> sounds;
int playerStepChannel = -1;
int musicChannel = -1;

std::chrono::_V2::system_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
std::chrono::_V2::system_clock::time_point lastTime = std::chrono::high_resolution_clock::now();