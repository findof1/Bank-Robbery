#pragma once
#include "types.h"
#include <vector>
#include <random>
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
    "./textures/spikeTrap.png",
    "./textures/stairs.png",
    "./textures/safeDoorBoss.png",
    "./textures/enemy.png",
    "./textures/bomb.png",
    "./textures/bullet.png",
    "./textures/key.png",
    "./textures/coin.png",
    "./textures/hammerEnemy.png",
    "./textures/spike.png",
    "./textures/drone.png",
    "./textures/goldBar.png",
    "./textures/swat.png",
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
int enemyMeleeCooldown = 1000;
int hammerEnemyMeleeCooldown = 2000;
int spikeTrapInterval = 2500;
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

namespace BossValues
{
    const int initialBossHealth = 1000;

    int generateStrafingTime()
    {
        std::mt19937 gen(rd());

        std::uniform_real_distribution<float> dist(0, 5000);

        float randomNum = dist(gen);
        return randomNum + 5000;
    }

    std::chrono::_V2::system_clock::time_point strafingTimer = std::chrono::high_resolution_clock::now();
    int strafingTime = generateStrafingTime();
    int strafeDir = 0;

    int generateChargeTime()
    {
        std::mt19937 gen(rd());

        std::uniform_real_distribution<float> dist(0, 2000);

        float randomNum = dist(gen);
        return randomNum + 5000;
    }
    std::chrono::_V2::system_clock::time_point chargeTimer = std::chrono::high_resolution_clock::now();
    int chargeTime = generateChargeTime();
    std::chrono::_V2::system_clock::time_point chargeDurationTimer = std::chrono::high_resolution_clock::now();
    int chargeDuration = 150;

    int shootingCooldown = 2000;

    std::chrono::_V2::system_clock::time_point bigAttackTimer = std::chrono::high_resolution_clock::now();
    int bigAttackTime = 10000;

    bool door1 = false;
    bool door2 = false;
    bool door3 = false;
    bool door4 = false;
}