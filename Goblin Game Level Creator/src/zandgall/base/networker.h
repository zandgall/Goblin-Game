#pragma once
#include <thread>
#include <WinSock2.h>
#include <nbt/nbt.hpp>
#define NETWORK_BUFFER_LENGTH 512
void init_networking();
typedef void (*message_handler)(SOCKET, nbt::compound);
namespace client {
	extern SOCKET connected_socket;

	extern message_handler on_message;
	extern void connect(std::string ip, std::string port);
	extern void send_message(nbt::compound message);
	extern void cleanup();
};
namespace server {
	extern message_handler on_message;
	extern SOCKET listen_socket;
	extern std::vector<SOCKET> clients;
	extern void connect(std::string port);
	extern void send_to(SOCKET, nbt::compound);
	extern void send_all(nbt::compound);
	extern void cleanup();
};