#include "networker.h"
#include <WS2tcpip.h>
#include <loadgz/gznbt.h>

message_handler client::on_message = nullptr;
SOCKET client::connected_socket = 0;
void init_networking() {
    WSADATA data;
    int res = WSAStartup(MAKEWORD(2, 2), &data);
    if (res)
        std::cout << "WSAStartup failed :( " << res << std::endl;
}

void handle_message() {
    while (true) {
        int res;
        do {
            char recvbuf[NETWORK_BUFFER_LENGTH];
            res = recv(client::connected_socket, recvbuf, NETWORK_BUFFER_LENGTH, 0);
            if (res > 0) {
                std::vector<char> inflated = std::vector<char>();
                nbt::inflate(recvbuf, res, &inflated);
                if (inflated.size() == 0) {
                    std::cout << "Data was not gzipped! " << recvbuf << std::endl;
                    continue;
                }
                nbt::compound data = nbt::compound();
                try {
                    data.load(&inflated[0], 0);
                }
                catch (std::exception e) {
                    std::cout << "Couldn't load nbt data " << e.what() << std::endl;
                    continue;
                };
                if (client::on_message)
                    client::on_message(client::connected_socket, data);
            }
            else if (res != 0)
                std::cout << "Err on recv " << WSAGetLastError() << std::endl;
        } while (res > 0);
    }
}
void client::connect(std::string ip, std::string port) {
    struct addrinfo* result = NULL, * ptr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int res = getaddrinfo(ip.c_str(), port.c_str(), &hints, &result);
    if (res) {
        std::cout << "Unable to get address info of " << ip << ":" << port << " Error: #" << res << std::endl;
        WSACleanup();
        return;
    }
    std::cout << "Got address info of " << ip << ":" << port << std::endl;


    connected_socket = INVALID_SOCKET;
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        connected_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connected_socket == INVALID_SOCKET) {
            std::cout << "Failed to connect to socket " << ptr << " " << WSAGetLastError() << std::endl;
            WSACleanup();
            return;
        }

        res = connect(connected_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (res == SOCKET_ERROR) {
            closesocket(connected_socket);
            connected_socket = INVALID_SOCKET;
            std::cout << "Checking next socket..." << std::endl;
            continue;
        }
        std::cout << "Found socket!" << std::endl;
        break;
    }
    
    std::cout << "Detaching message handler thread!" << std::endl;
    std::thread(handle_message).detach();

    freeaddrinfo(result);
    if (connected_socket == INVALID_SOCKET) {
        std::cout << "Unable to connect to server!" << std::endl;
        WSACleanup();  
        return;
    }
    std::cout << "Successfully Connected to Server "<<ip<<":"<<port << std::endl;
}
void client::send_message(nbt::compound message) {
    std::vector<char> buffer = std::vector<char>();
    std::vector<char> deflated = std::vector<char>();
    message.write(buffer);
    nbt::deflate(&buffer[0], buffer.size(), &deflated, 9);
    int res;
    //do { 
        res = send(connected_socket, &deflated[0], deflated.size(), 0);
        if (res == SOCKET_ERROR) {
            std::cout << "Failed to send buffer :( " << WSAGetLastError() << std::endl;
            closesocket(connected_socket);
            WSACleanup();
            return;
        }
    //} while (res > 0);
}
void client::cleanup() {
    int res = shutdown(connected_socket, SD_SEND);
    closesocket(connected_socket);
    WSACleanup();
    if (res == SOCKET_ERROR)
        std::cout << "Shutdown error " << WSAGetLastError() << std::endl;
    return;
}

std::vector<SOCKET> server::clients = std::vector<SOCKET>();
message_handler server::on_message = nullptr;
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
                std::vector<char> inflated = std::vector<char>();
                nbt::inflate(recvbuf, res, &inflated);
                if (inflated.size() == 0) {
                    std::cout << "server: Received data was not GZipped" << std::endl;
                    continue;
                }
                nbt::compound data = nbt::compound();
                try {
                    data.load(&inflated[0], 0);
                }
                catch (std::exception e) {
                    std::cout << "server: Error loading nbt data " << e.what() << std::endl;
                    continue;
                }
                if (server::on_message)
                    server::on_message(server::clients[i], data);
            }
            else if (res != 0) {
                std::cout << "Receive failed! Error: #" << WSAGetLastError() << " Disconnecting socket";
                server::clients.erase(server::clients.begin()+i);
                closesocket(server::clients[i]);
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
            std::vector<char> inflated = std::vector<char>();
            nbt::inflate(recvbuf, res, &inflated);
            if (inflated.size() == 0) {
                std::cout << "server: Received data was not GZipped" << std::endl;
                continue;
            }
            nbt::compound data = nbt::compound();
            try {
                data.load(&inflated[0], 0);
            }
            catch (std::exception e) {
                std::cout << "server: Error loading nbt data " << e.what() << std::endl;
                continue;
            }
            if (server::on_message)
                server::on_message(client, data);
        }
        else if (res != 0) {
            std::cout << "Receive failed! Error: #" << WSAGetLastError() << " Disconnecting socket";
            server::clients.erase(std::find(server::clients.begin(), server::clients.end(), client));
            closesocket(client);
            return;
        }
    } while (res >= 0);
}

void accept_clients() {
    while (true) {
        int res = listen(server::listen_socket, SOMAXCONN);
        if (res == SOCKET_ERROR) {
            std::cout << "Listen bad :( " << WSAGetLastError() << std::endl;
            closesocket(server::listen_socket);
            WSACleanup();
            return;
        }
        std::cout << "Found socket connection! Trying to accept..." << std::endl;

        SOCKET client;
        client = accept(server::listen_socket, NULL, NULL);
        if (client == INVALID_SOCKET) {
            std::cout << "Client accept failed :( " << WSAGetLastError() << std::endl;
            closesocket(server::listen_socket);
            WSACleanup();
            return;
        }
        std::cout << "Accepted new client!" << std::endl;
        server::clients.push_back(client);
        std::thread(client_handle, client).detach();
    }
}

void server::connect(std::string port) {
    struct addrinfo hints, * result;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    int res = getaddrinfo(NULL, port.c_str(), &hints, &result);
    if (res) {
        std::cout << "Could not get Addr info of localhost:" << port << " #" << res << std::endl;
        WSACleanup();
        return;
    }
    std::cout << "Got address info of localhost:" << port << std::endl;
    listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listen_socket == INVALID_SOCKET) {
        std::cout << "Socket brokey " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return;
    }
    std::cout << "Created listen socket" << std::endl;

    res = bind(listen_socket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);
    if (res == SOCKET_ERROR) {
        std::cout << "Bind brokey " << res << std::endl;
        closesocket(listen_socket);
        WSACleanup();
        return;
    }
    std::cout << "Bound listen socket" << std::endl;


    std::thread(accept_clients).detach();
    //std::thread(client_interact).detach();
    std::cout << "Detached listen and accept threads" << std::endl;
}

void server::send_to(SOCKET socket, nbt::compound data) {
    std::vector<char> buffer = std::vector<char>();
    std::vector<char> deflated = std::vector<char>();
    data.write(buffer);
    nbt::deflate(&buffer[0], buffer.size(), &deflated, 9);
    int res;
    //do {
        res = send(socket, &deflated[0], deflated.size(), 0);
        if (res == SOCKET_ERROR) {
            std::cout << "Failed to send buffer :( " << WSAGetLastError() << std::endl;
            closesocket(socket);
            WSACleanup();
            return;
        }
    //} while (res >= 0);
}

void server::send_all(nbt::compound data) {
    std::vector<char> buffer = std::vector<char>();
    std::vector<char> deflated = std::vector<char>();
    data.write(buffer);
    nbt::deflate(&buffer[0], buffer.size(), &deflated, 9);
    for (auto client = clients.begin(); client != clients.end(); client++) {
        int res;
        //do {
            res = send(*client, &deflated[0], deflated.size(), 0);
            if (res == SOCKET_ERROR) {
                std::cout << "Failed to send message to client, closing client, err: #" << WSAGetLastError() << std::endl;
                clients.erase(client);
                closesocket(*client);
                --client;
            }
        //} while (res > 0);
    }
}

void server::cleanup() {
    closesocket(listen_socket);
}