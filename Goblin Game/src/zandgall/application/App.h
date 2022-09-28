#pragma once
#include <glm/glm.hpp>
#include "../appwork/appwork.h"
#define TILE_SIZE 16
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 240
#define SCALE_UP_FACTOR 3
#define FRAND ((float)rand() / (float)RAND_MAX)
extern bool IS_SERVER, IS_CONNECTED;
extern const bool AABB(glm::vec4 a, glm::vec4 b);
extern void send_message(char* bytes, size_t size);
enum message_types {
	connect_message, new_client, player_state, player_velocity, player_input, player_flag, player_position, ping, pong, object_update
};

class App {
public:
	static App* instance;
	app::window* window;
	App();

	~App() {

	}

	void tick(double delta);
	void render();
};

