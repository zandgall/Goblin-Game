#ifndef PLAYER_H
#define PLAYER_H
#include <glm/glm.hpp>
#include "World.h"
#include "../appwork/appwork.h"
class Player {
public:
	enum {
		respawn_flag = 1
	};
	glm::vec2 pos, vel;
	float jumpOffsetting = 0, frictionOffsetting = 0;
	float index = 0;
	double creation = 0;
	bool flip = false;

	Object* touching_object = nullptr;

	glm::bvec4 control = glm::bvec4(0), pcontrol = glm::bvec4(0);
	glm::ivec4 collisions = glm::ivec4(0);
	uint8_t flags = 0;

	int object_id = -1;

	Player();
	~Player() {

	}
	void tick(float delta, float nextDelta, World& world, app::window* window);
	void render();
	void respawn(glm::vec2 spawnpoint);
protected:
	
private:
};
#endif