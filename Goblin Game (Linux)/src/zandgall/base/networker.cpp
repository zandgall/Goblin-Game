#include "networker.h"
#ifdef _WIN32
#include <WS2tcpip.h>
#else
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#endif
#include <iostream>
#include <algorithm>

message_handler client::on_message = nullptr;
SOCKET client::connected_socket = 0;
void init_networking() {
#ifdef _WIN32
    WSADATA data;
    int res = WSAStartup(MAKEWORD(2, 2), &data);
    if (res)
        std::cout << "WSAStartup failed :( " << res << std::endl;
#endif
}

void handle_message() {
    while (true) {
        int res;
        do {
            char recvbuf[NETWORK_BUFFER_LENGTH];
            res = recv(client::connected_socket, recvbuf, NETWORK_BUFFER_LENGTH, 0);
            if (res > 0) {
                if (client::on_message)
                    client::on_message(client::connected_socket, recvbuf, res);
            }
            else if (res != 0)
#ifdef _WIN32
                std::cout << "Err on recv " << WSAGetLastError() << std::endl;
#else
                std::cout << "Err on recv " << strerror(errno) << std::endl;
#endif
        } while (res > 0);
    }
}
void client::connect(std::string ip, std::string port) {
    struct addrinfo* result = NULL, * ptr = NULL, hints;
#ifdef _WIN32
    ZeroMemory(&hints, sizeof(hints));
#else
    memset(&hints, 0, sizeof(hints));
#endif
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int res = getaddrinfo(ip.c_str(), port.c_str(), &hints, &result);
    if (res) {
        std::cout << "Unable to get address info of " << ip << ":" << port << " Error: #" << res << std::endl;
#ifdef _WIN32
        WSACleanup();  
#endif
        return;
    }
    std::cout << "Got address info of " << ip << ":" << port << std::endl;


    connected_socket = -1;
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        connected_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connected_socket < 0) {
#ifdef _WIN32
            std::cout << "Failed to connect to socket " << ptr << " " << WSAGetLastError() << std::endl;
            WSACleanup();
#else
            std::cout << "Failed to connect to socket " << ptr << " " << strerror(errno) << std::endl;
#endif
            return;
        }

        res = connect(connected_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (res < 0) {
#ifdef _WIN32
            closesocket(connected_socket);
#else
            shutdown(connected_socket, 2);
#endif
            connected_socket = -1;
            std::cout << "Checking next socket..." << std::endl;
            continue;
        }
        std::cout << "Found socket!" << std::endl;
        break;
    }
    
    std::cout << "Detaching message handler thread!" << std::endl;
    std::thread(handle_message).detach();

    freeaddrinfo(result);
    if (connected_socket < 0) {
        std::cout << "Unable to connect to server!" << std::endl;
#ifdef _WIN32
        WSACleanup();  
#endif
        return;
    }
    std::cout << "Successfully Connected to Server "<<ip<<":"<<port << std::endl;
}
void client::send_message(char* bytes, size_t size) {
    int res;
    //do { 
        res = send(connected_socket, bytes, size, 0);
        if (res < 0) {
#ifdef _WIN32
            std::cout << "Failed to send buffer :( " << WSAGetLastError() << std::endl;
            closesocket(connected_socket);
            WSACleanup();
#else
            std::cout << "Failed to send buffer :( " << strerror(errno) << std::endl;
            shutdown(connected_socket, 2);
#endif
            return;
        }
    //} while (res > 0);
}
void client::cleanup() {
    int res = shutdown(connected_socket, 1);
#ifdef _WIN32
    closesocket(connected_socket);
    WSACleanup();
    if (res == SOCKET_ERROR)
        std::cout << "Shutdown error " << WSAGetLastError() << std::endl;
#else
    if (res < 0)
        std::cout << "Shutdown error " << strerror(errno) << std::endl;
#endif
    return;
}

std::vector<SOCKET> server::clients = std::vector<SOCKET>();
message_handler server::on_message = nullptr;
socket_handler server::on_connect = nullptr;
SOCKET server::listen_socket = 0;

void client_interact() {
    while (true) {
        for (int i = 0; i < server::clients.size(); i++) {
            if (!server::clients[i]) {
                std::cout << "Client doesn't exist" << std::endl;
                continue;
            }
            int res;
            char recvbuf[NETWORK_BUFFER_LENGTH];
            std::cout << "Listening to client " << server::clients[i] << std::endl;
            //do {
            res = recv(server::clients[i], recvbuf, NETWORK_BUFFER_LENGTH, 0);
            if (res > 0) {
                if (server::on_message)
                    server::on_message(server::clients[i], recvbuf, res);
            }
            else if (res != 0) {
#ifdef _WIN32
                std::cout << "Receive failed! Error: #" << WSAGetLastError() << " Disconnecting socket" << std::endl;
                closesocket(server::clients[i]);
#else  
                std::cout << "Receive failed! Error: " << strerror(errno) << " Disconnecting socket" << std::endl;
                shutdown(server::clients[i], 2);
#endif
                server::clients.erase(server::clients.begin()+i);
                --i;
            }
            //} while (res > 0);
        }
    }
}

void client_handle(SOCKET client) {
    int res;
    char recvbuf[NETWORK_BUFFER_LENGTH];
    std::cout << "Listening to client " << client << std::endl;
    do {
        res = recv(client, recvbuf, NETWORK_BUFFER_LENGTH, 0);
        if (res > 0) {
            if (server::on_message)
                server::on_message(client, recvbuf, res);
        }
        else if (res != 0) {
#ifdef _WIN32
            std::cout << "Receive failed! Error: #" << WSAGetLastError() << " Disconnecting socket";
            closesocket(client);
#else
            std::cout << "Receive falied! Error: " << strerror(errno) << " Disconnecting socket";
            shutdown(client, 2);
#endif
            server::clients.erase(std::find(server::clients.begin(), server::clients.end(), client));
            return;
        }
    } while (res >= 0);
}

void accept_clients() {
    while (true) {
        int res = listen(server::listen_socket, SOMAXCONN);
        if (res < 0) {
#ifdef _WIN32
            std::cout << "Listen bad :( " << WSAGetLastError() << std::endl;
            closesocket(server::listen_socket);
            WSACleanup();
#else
            std::cout << "Listen bad :( " << strerror(errno) << std::endl;
#endif
            return;
        }
        std::cout << "Found socket connection! Trying to accept..." << std::endl;

        SOCKET client;
        client = accept(server::listen_socket, NULL, NULL);
        if (client < 0) {
#ifdef _WIN32
            std::cout << "Client accept failed :( " << WSAGetLastError() << std::endl;
            closesocket(server::listen_socket);
            WSACleanup();
#else
            std::cout << "Client accept failed :( " << strerror(errno) << std::endl;
#endif
            return;
        }
        std::cout << "Accepted new client!" << std::endl;
        if (server::on_connect)
            server::on_connect(client);
        server::clients.push_back(client);
        std::thread(client_handle, client).detach();
    }
}

void server::connect(std::string port) {
    struct addrinfo hints, * result;
#ifdef _WIN32
    ZeroMemory(&hints, sizeof(hints));
#else
    memset(&hints, 0, sizeof(hints));
#endif
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    int res = getaddrinfo(NULL, port.c_str(), &hints, &result);
    if (res) {
        std::cout << "Could not get Addr info of localhost:" << port << " #" << res << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }
    std::cout << "Got address info of localhost:" << port << std::endl;
    listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listen_socket < 0) {
        freeaddrinfo(result);
#ifdef _WIN32
        std::cout << "Socket brokey " << WSAGetLastError() << std::endl;
        WSACleanup();
#else
        std::cout << "Socket brokey " << strerror(errno) << std::endl;
#endif
        return;
    }
    std::cout << "Created listen socket" << std::endl;

    res = bind(listen_socket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);
    if (res < 0) {
        std::cout << "Bind brokey " << res << std::endl;
        cleanup();
        return;
    }
    std::cout << "Bound listen socket" << std::endl;


    std::thread(accept_clients).detach();
    //std::thread(client_interact).detach();
    std::cout << "Detached listen and accept threads" << std::endl;
}

void server::send_to(SOCKET socket, char* bytes, size_t size) {
    int res;
    //do {
        res = send(socket, bytes, size, 0);
        if (res < 0) {
#ifdef _WIN32
            std::cout << "Failed to send buffer :( " << WSAGetLastError() << std::endl;
#else
            std::cout << "Failed to send buffer :( " << strerror(errno) << std::endl;
#endif
            //closesocket(socket);
            //WSACleanup();
            return;
        }
    //} while (res >= 0);
}

void server::send_all(char* bytes, size_t size) {
    for (auto client = clients.begin(); client != clients.end(); client++) {
        int res;
        //do {
            res = send(*client, bytes, size, 0);
            if (res < 0) {
#ifdef _WIN32
                std::cout << "Failed to send message to client, closing client, err: #" << WSAGetLastError() << std::endl;
                closesocket(*client);
#else
                std::cout << "Failed to send message to client, closing client, err: " << strerror(errno) << std::endl;
                shutdown(*client, 2);
#endif
                clients.erase(client);
                --client;
            }
        //} while (res > 0);
    }
}

void server::cleanup() {
#ifdef _WIN32
    closesocket(listen_socket);
    WSACleanup();
#else
    shutdown(listen_socket, 2);
#endif
}