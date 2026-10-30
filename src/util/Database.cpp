#include "Database.hpp"
#include <iostream>

Database::Database(const std::string &db_path) {
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "[DB Error] Failed to open database: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
    }
}

Database::~Database() {
    if (db) {
        sqlite3_close(db);
    }
}

bool Database::init() {
    if (!db) return false;

    const char *create_table_sql =
        "CREATE TABLE IF NOT EXISTS players ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT NOT NULL,"
        "password_hash TEXT NOT NULL,"
        "pos_x REAL NOT NULL DEFAULT 32.0,"
        "pos_y REAL NOT NULL DEFAULT 120.0,"
        "pos_z REAL NOT NULL DEFAULT 32.0,"
        "health INTEGER NOT NULL DEFAULT 20"
        ");";

    char *err_msg = nullptr;
    if (sqlite3_exec(db, create_table_sql, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "[DB Error] Failed to table: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}

bool Database::register_user(const std::string &username, const std::string &password_hash, uint32_t &out_id) {
    if (!db) return false;

    const char *sql = "INSERT INTO players (username, password_hash) VALUES (?, ?);";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    if (success) {
        out_id = static_cast<uint32_t>(sqlite3_last_insert_rowid(db));
    } else {
        std::cerr << "[DB Error] Registration failed (Username might already exist): " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return success;
}

bool Database::verify_user(const std::string &username, const std::string &password_hash, PlayerData &out_player) {
    if (!db) return false;

    const char *sql = "SELECT id, pos_x, pos_y, pos_z, health FROM players WHERE username = ? AND password_hash = ?;";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_TRANSIENT);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out_player.id = static_cast<uint32_t>(sqlite3_column_int(stmt, 0));
        out_player.username = username;
        out_player.password_hash = password_hash;
        out_player.position.x = static_cast<float>(sqlite3_column_double(stmt, 1));
        out_player.position.y = static_cast<float>(sqlite3_column_double(stmt, 2));
        out_player.position.z = static_cast<float>(sqlite3_column_double(stmt, 3));
        out_player.health = static_cast<uint8_t>(sqlite3_column_int(stmt, 4)); 
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

bool Database::save_player(const PlayerData &player) {
    if (!db) return false;
  
    const char *sql =
        "UPDATE players SET "
        "pos_x = ?, "
        "pos_y = ?, "
        "pos_z = ?, "
        "health = ? "
        "WHERE id = ?;";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DB Error] Prepare statement failed: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_double(stmt, 1, player.position.x);
    sqlite3_bind_double(stmt, 2, player.position.y);
    sqlite3_bind_double(stmt, 3, player.position.z);
    sqlite3_bind_int(stmt, 4, player.health);
    sqlite3_bind_int(stmt, 5, static_cast<int>(player.id));

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!success) {
        std::cerr << "[DB Error] Save execution failed: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "[Server] Successfully saved position (" 
            << player.position.x << ", " << player.position.y << ", " << player.position.z 
            << ") for user ID: " << player.id << std::endl;
    }

    sqlite3_finalize(stmt);
    return success;
}

bool Database::load_player(uint32_t id, PlayerData &out_player) {
    if (!db) return false;

    const char *sql = "SELECT username, pos_x, pos_y, pos_z, health FROM players WHERE id = ?;";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, static_cast<int>(id));

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out_player.id = id;
        out_player.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        out_player.position.x = static_cast<float>(sqlite3_column_double(stmt, 1));
        out_player.position.y = static_cast<float>(sqlite3_column_double(stmt, 2));
        out_player.position.z = static_cast<float>(sqlite3_column_double(stmt, 3));
        out_player.health = static_cast<uint8_t>(sqlite3_column_int(stmt, 4));
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}
