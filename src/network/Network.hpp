#pragma once

#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <iostream>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    
    using socket_t = SOCKET;
    constexpr socket_t INVALID_SOCK = INVALID_SOCKET;
    #define CLOSE_SOCK(s) closesocket(s)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <errno.h>
    
    using socket_t = int;
    constexpr socket_t INVALID_SOCK = -1;
    #define CLOSE_SOCK(s) close(s)
#endif

inline void set_non_blocking(socket_t sock) {
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, FGETFL, 0);
    fcntl(sock, FSETFL, flags | O_NONBLOCK);
#endif
}

enum class TransportType {
    TCP, UDP
};

struct Connection {
    virtual ~Connection() = default;
    virtual int send(const char *buffer, size_t length) = 0;
    virtual int recv(char *buffer, size_t length) = 0;
    virtual void close() = 0;
    virtual bool is_open() const = 0;
};

struct Server {
    virtual ~Server() = default;
    virtual bool start() = 0;
    virtual void update() = 0;
    virtual void stop() = 0;
    virtual int get_port() const = 0;
    virtual bool is_open() const = 0;
    virtual const std::vector<std::shared_ptr<Connection>> &get_clients() const = 0;
};

struct TcpConnection : public Connection {
    explicit TcpConnection(socket_t descriptor)
        : sock(descriptor), connected(descriptor != INVALID_SOCK) {
        if (connected) {
            set_non_blocking(sock);
        }
    }

    ~TcpConnection() override { 
        close(); 
    }

    int send(const char *buffer, size_t length) override {
        if (!connected) return -1;
        return ::send(sock, buffer, static_cast<int>(length), 0);
    }

    int recv(char *buffer, size_t length) override {
        if (!connected) return -1;
        int bytes = ::recv(sock, buffer, static_cast<int>(length), 0);

        if (bytes == 0) {
            connected = false;
        } else if (bytes < 0) {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK) return 0;
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
#endif
            connected = false;
        }
        return bytes;
    }

    void close() override {
        if (sock != INVALID_SOCK) {
            CLOSE_SOCK(sock);
            sock = INVALID_SOCK;
        }
        connected = false;
    }

    bool is_open() const override { return connected; }

private:
    socket_t sock = INVALID_SOCK;
    bool connected = false;
};

struct TcpServer : public Server {
    explicit TcpServer(int port) : port(port) {}
    ~TcpServer() override { stop(); }

    bool start() override {
        listen_sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sock == INVALID_SOCK) return false;

        int opt = 1;
        ::setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR,
            reinterpret_cast<const char*>(&opt), sizeof(opt));
        set_non_blocking(listen_sock);

        sockaddr_in addr {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (::bind(listen_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            return false;
        }

        if (::listen(listen_sock, SOMAXCONN) < 0) {
            return false;
        }

        return true;
    }

    void update() override {
        if (listen_sock == INVALID_SOCK) return;

        sockaddr_in client_addr {};
        socklen_t addr_len = sizeof(client_addr);
        socket_t client_sock = ::accept(listen_sock, 
            reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
        if (client_sock != INVALID_SOCK) {
            clients.push_back(std::make_shared<TcpConnection>(client_sock));
        }

        clients.erase(
            std::remove_if(clients.begin(), clients.end(),
                [](const std::shared_ptr<Connection> &conn) { return !conn->is_open(); }),
            clients.end());
    }    

    void stop() override {
        clients.clear();
        if (listen_sock != INVALID_SOCK) {
            CLOSE_SOCK(listen_sock);
            listen_sock = INVALID_SOCK;
        }
    }

    int get_port() const override { 
        return port; 
    }

    bool is_open() const override { 
        return listen_sock != INVALID_SOCK;
    }

    const std::vector<std::shared_ptr<Connection>> &get_clients() const override {
        return clients;
    }

private:    
    socket_t listen_sock = INVALID_SOCK;
    int port = 0;
    std::vector<std::shared_ptr<Connection>> clients;
};

struct UdpConnection : public Connection {
    UdpConnection(socket_t descriptor, const sockaddr_in &peer)
        : sock(descriptor), peer_addr(peer), open_state(descriptor != INVALID_SOCK) {
        if (open_state) {
            set_non_blocking(sock);
        }
    }

    ~UdpConnection() override { 
        close();
    }

    int send(const char *buffer, size_t length) override {
        if (!open_state) return -1;
        return ::sendto(sock, buffer, static_cast<int>(length), 0,
            reinterpret_cast<const sockaddr*>(&peer_addr), sizeof(peer_addr));
    }

    int recv(char *buffer, size_t length) override {
        if (!open_state) return -1;
        socklen_t len = sizeof(peer_addr);
        int bytes = ::recvfrom(sock, buffer, static_cast<int>(length), 0,
            reinterpret_cast<sockaddr*>(&peer_addr), &len);

        if (bytes < 0) {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEWOULDBLOCK) return 0;
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
#endif
            return -1;
        }
        return bytes;
    }

    void close() override {
        if (sock != INVALID_SOCK) {
            CLOSE_SOCK(sock);
            sock = INVALID_SOCK;
        }
        open_state = false;
    }

    bool is_open() const override { return open_state; }

private:    
    socket_t sock = INVALID_SOCK;
    sockaddr_in peer_addr {};
    bool open_state = false;
};

struct UdpServer : public Server {
    explicit UdpServer(int port) : port(port) {}

    ~UdpServer() override {
        stop();
    }

    bool start() override {
        sock = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == INVALID_SOCK) return false;

        int opt = 1;
        ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
            reinterpret_cast<const char*>(&opt), sizeof(opt));
        set_non_blocking(sock);

        sockaddr_in addr {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (::bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            CLOSE_SOCK(sock);
            sock = INVALID_SOCK;
            return false;
        }
        return true;
    }

    void update() override {
        // Datagram processing loop
    }

    void stop() override {
        active_peers.clear();
        if (sock != INVALID_SOCK) {
            CLOSE_SOCK(sock);
            sock = INVALID_SOCK;
        }
    }

    int get_port() const override {
        return port;
    }

    bool is_open() const override {
        return sock != INVALID_SOCK;
    }

    const std::vector<std::shared_ptr<Connection>> &get_clients() const override {
        return active_peers;
    }

private:
    socket_t sock = INVALID_SOCK;
    int port = 0;
    std::vector<std::shared_ptr<Connection>> active_peers;
};

struct Network {
    Network() = default;

    std::shared_ptr<Server> create_tcp_server(int port) {
        auto server = std::make_shared<TcpServer>(port);
        if (server->start()) {
            servers.push_back(server);
            return server;
        }
        return nullptr;
    }

    std::shared_ptr<Server> create_udp_server(int port) {
        auto server = std::make_shared<UdpServer>(port);
        if (server->start()) {
            servers.push_back(server);
            return server;
        }
        return nullptr;
    }

    void add_connection(std::shared_ptr<Connection> conn) {
        if (conn && conn->is_open()) {
            outbound_connections.push_back(conn);
        }
    }

    void update() {
        for (auto &server : servers) {
            server->update();
        }

        outbound_connections.erase(
            std::remove_if(outbound_connections.begin(), outbound_connections.end(),
                [](const std::shared_ptr<Connection> &conn) { return !conn->is_open(); }),
            outbound_connections.end());
    }

    const std::vector<std::shared_ptr<Server>> &get_servers() const {
        return servers;
    }

    const std::vector<std::shared_ptr<Connection>> &get_connections() const {
        return outbound_connections;
    }

private:
    std::vector<std::shared_ptr<Server>> servers;
    std::vector<std::shared_ptr<Connection>> outbound_connections;
};