#pragma once

#include <cmath>
#include <cwchar>
#include <sqlite3.h>
#include <string>
#include <glm/glm.hpp>
#include <iostream>

struct PlayerData {
    PlayerData() = default;

    PlayerData(const PlayerData &other) = delete;
    PlayerData(PlayerData &&other) = default;
    PlayerData &operator=(const PlayerData &other) = delete;
    PlayerData &operator=(PlayerData &&other) = default;

    uint32_t id = 0;
    std::string username;
    std::string password_hash;
    glm::vec3 position { 32.0f, 120.0f, 32.0f };
    uint8_t health = 20;
};

struct Database {
    Database() = default;
    
    explicit Database(const std::string &db_path);
    ~Database();

    Database(const Database &other) = delete;
    Database(Database &&other) = default;
    Database &operator=(const Database &other) = delete;
    Database &operator=(Database &&other) = default;

    bool init();
    bool register_user(const std::string &username, const std::string &password_hash, uint32_t &out_id);
    bool verify_user(const std::string &username, const std::string &password_hash, PlayerData &out_player);
    bool save_player(const PlayerData &player);
    bool load_player(uint32_t id, PlayerData &out_player);

private:
    sqlite3 *db = nullptr;
};
