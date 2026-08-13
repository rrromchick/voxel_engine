#include "Network.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <memory>

int main(int argc, char *argv[]) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    auto network = std::make_unique<Network>();
    auto server = network->create_tcp_server(8080);
    if (!server) {
        std::cout << "Failed to start server on port 8080\n";
        return 1;
    }

    std::cout << "Server active on port 8080. Waiting for clients...\n";
    char buffer[512];

    while (server->is_open()) {
        network->update();

        for (const auto &client : server->get_clients()) {
            int bytes = client->recv(buffer, sizeof(buffer) - 1);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                std::cout << "Received: " << buffer << "\n";

                for (const auto &peer : server->get_clients()) {
                    if (peer != client) {
                        peer->send(buffer, bytes);
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}