#pragma once
#include <thread>
#include <WinSock2.h>
#define NETWORK_BUFFER_LENGTH 512
void init_networking();
typedef void (*message_handler)(SOCKET, char* bytes, size_t size);
typedef void (*socket_handler)(SOCKET);
namespace client {
	extern SOCKET connected_socket;

	extern message_handler on_message;
	extern void connect(std::string ip, std::string port);
	extern void send_message(char* bytes, size_t size);
	extern void cleanup();
};
namespace server {
	extern message_handler on_message;
	extern socket_handler on_connect;
	extern SOCKET listen_socket;
	extern std::vector<SOCKET> clients;
	extern void connect(std::string port);
	extern void send_to(SOCKET, char* bytes, size_t size);
	extern void send_all(char* bytes, size_t size);
	extern void cleanup();
};