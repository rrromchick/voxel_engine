#pragma once

#include "State.hpp"
#include "Database.hpp"
#include "Network.hpp"
#include <memory>
#include <string>

struct StateMainMenu : public State {
    explicit StateMainMenu(std::shared_ptr<Connection> conn);

    void init() override;
    void destroy() override;
    void update() override;
    void render() override;

    bool is_finished() const { return switch_to_game; }
    PlayerData release_player_data() { return std::move(authenticated_player); }

private:
    std::shared_ptr<Connection> server_conn;
    Database db;

    char username_buf[64] = "";
    char password_buf[64] = "";
    std::string error_message = "";
    bool switch_to_game = false;
    PlayerData authenticated_player;
};
