#include "Player.h"
#include "../base/handler.h"
#include "../base/glhelper.h"
#include "App.h"
#include "../appwork/appwork.h"
#include "Assets.h"
#include <vector>
std::vector<glm::vec3> dashGhosts = std::vector<glm::vec3>();

Player::Player() {
	pos = glm::vec2(100);
	vel = glm::vec2(0);
}

void Player::tick(float delta, float nextDelta, World& world, app::window* window) {
	collisions -= glm::ivec4(1);
	collisions.x = std::max(collisions.x, 0);
	collisions.y = std::max(collisions.y, 0);
	collisions.z = std::max(collisions.z, 0);
	collisions.w = std::max(collisions.w, 0);

	touching_object = nullptr;

	for (int i = 0; i < world.objects.size(); i++) {
		if (i == object_id)
			continue;
 		Object& o = world.objects[i];
		if (!world.object_definitions[o.object].solid)
			continue;
		glm::vec2 v = glm::vec2(0);
		if (o.data.has("vel"))
			v = glm::vec2(o.data["vel"]["x"].f(), o.data["vel"]["y"].f());
		if (vel.y >= v.y && AABB(glm::vec4(o.pos.x+v.x*delta, o.pos.y + v.y * delta, world.object_definitions[o.object].size), glm::vec4(pos.x + 11 + vel.x * delta, pos.y + vel.y * delta + 32 + 1, 9, vel.y * delta))) {
			pos.y = o.pos.y - 32;
			collisions.w = 5;
			vel.y = v.y;
			touching_object = &world.objects[i];
		}

		if (v.y >= vel.y && AABB(glm::vec4(o.pos.x + v.x * delta + 1, o.pos.y + world.object_definitions[o.object].size.y + 1, world.object_definitions[o.object].size.x-2, v.y * delta), glm::vec4(pos.x+12 + vel.x * delta, pos.y+8 + vel.y * delta, 8, 24))) {
			o.pos.y = pos.y - world.object_definitions[o.object].size.y + 8;
			v.y = vel.y;
		}

		if (vel.x > v.x && AABB(glm::vec4(o.pos + v * delta + glm::vec2(0, 3), world.object_definitions[o.object].size.x, 8), glm::vec4(pos.x + 12 + vel.x * delta, pos.y + vel.y * delta + 8 + 3, 8, 8))) {
			pos.x = o.pos.x - 32 + 12;
			collisions.z = 1;
			vel.x = v.x;
			touching_object = &world.objects[i];
		}
		if (vel.x < v.x && AABB(glm::vec4(o.pos + v * delta, world.object_definitions[o.object].size), glm::vec4(pos.x + 12 + vel.x * delta, pos.y + vel.y*delta + 8 + 3, 8, 18))) {
			pos.x = o.pos.x + world.object_definitions[o.object].size.x - 12;
			collisions.x = 1;
			vel.x = v.x;
			touching_object = &world.objects[i];
		}
	}
	
	if (control.x) {
		vel.x -= 50;
	} if (control.z) {
		vel.x += 50;
	} if (control.w) {
		vel.y += 50;
	} if (control.y) {
		if (collisions.w && !pcontrol.y) {
			vel.y = -500.f;
			//local_pos = pos;
			jumpOffsetting = 5;
			collisions.w = 0;
		}
		jumpOffsetting += 1.8;
	}
	jumpOffsetting = std::max(jumpOffsetting - 2, 0.f);
	frictionOffsetting = std::max(frictionOffsetting - delta * 5.f, 0.f);

	if (collisions.w < 5 && frictionOffsetting == 0)
		vel.y += std::max(50 - jumpOffsetting * 10, 10.f);
	vel -= vel * std::min(delta * 10, 1.f) * glm::vec2(1.0 - frictionOffsetting, 0.2);
	if (abs(vel.x) < 0.1)
		vel.x = 0;
	if (abs(vel.y) < 0.1)
		vel.y = 0;

	pos += vel * delta;
	if (vel.x > 0)
		flip = true;
	else if (vel.x < 0)
		flip = false;

	if (vel.y >= 0 && (world.solid(pos.x + 1 - vel.x * nextDelta + 12, pos.y + 32) || world.solid(pos.x + 32 - 12 - 1 - vel.x * nextDelta, pos.y + 32))) {
		pos.y = floor((pos.y+32)/TILE_SIZE) * TILE_SIZE - 32;
		collisions.w = 5;
		vel.y = 0;
	}
	if (vel.x >= 0 && (world.solid(pos.x + 32-12, pos.y + 1 - vel.y * nextDelta + 8) || world.solid(pos.x + 32 - 12, pos.y + TILE_SIZE - vel.y * nextDelta)) || world.solid(pos.x + 32-12, pos.y + 32 - 1 - vel.y * nextDelta)) {
		pos.x = floor((pos.x + 32-12) / TILE_SIZE) * TILE_SIZE - 32 + 12;
		collisions.z = 1;
		vel.x = 0;
	}
	if (vel.x <= 0 && (world.solid(pos.x+12, pos.y + 1 + 8 - vel.y*nextDelta) || world.solid(pos.x + 12, pos.y + TILE_SIZE - vel.y * nextDelta)) || world.solid(pos.x + 12, pos.y + 32 - 1 - vel.y * nextDelta)) {
		pos.x = floor((pos.x + 12) / TILE_SIZE) * TILE_SIZE - 12 + TILE_SIZE;
	 	collisions.x = 1;
		vel.x = 0;
	}
	if (vel.y <= 0 && (world.solid(pos.x+1+12, pos.y + 8) || world.solid(pos.x + 32 - 12 - 1, pos.y + 8))) {
		pos.y = floor((pos.y + 8) / TILE_SIZE) * TILE_SIZE - 8 + TILE_SIZE;
		collisions.y = 1;
		vel.y = 0;
	}


	if (pos.y > 500 || flags & respawn_flag) {
		respawn(world.spawn);
		flags = 0;
		return; 
	}
	if (collisions.w)
		index += abs(vel.x) * delta * 0.02;
	flags = 0;
	if (object_id != -1) {
		world.objects[object_id].pos = pos+glm::vec2(12, 8);
		world.objects[object_id].data["vel"]["x"].ft().data = vel.x;
		world.objects[object_id].data["vel"]["y"].ft().data = vel.y;
	}
}
void Player::render() {
	if (creation == 0)
		creation = glfwGetTime();
	glUseProgram(_getGLShader("goblin shader"));

	uniVec("col", glm::vec4(sin(glfwGetTime() - creation), cos(glfwGetTime() - creation), sin((glfwGetTime() - creation)/3.14159265359), 1));
	uniInt("sheet", 0);
	uniInt("coords", 1);
	uniInt("color_mask", 2);
	uniFloat("index", index);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, goblin_sheet);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, color_mask);
	glActiveTexture(GL_TEXTURE1);
	//if (!IS_SERVER && IS_CONNECTED && object_id == 0) {
	//	if (flip)
	//		uniMat("transform", rect(local_pos.x * 3 + 32 * 3, local_pos.y * 3, -32 * 3, 32 * 3));
	//	else uniMat("transform", rect(local_pos.x * 3, local_pos.y * 3, 32 * 3, 32 * 3));
	//	if (abs(local_vel.x) <= 1)
	//		glBindTexture(GL_TEXTURE_2D, still);
	//	else glBindTexture(GL_TEXTURE_2D, run);
	//} 
	//else {
		if (flip)
			uniMat("transform", rect(pos.x * SCALE_UP_FACTOR + 32 * SCALE_UP_FACTOR, pos.y * SCALE_UP_FACTOR, -32 * SCALE_UP_FACTOR, 32 * SCALE_UP_FACTOR));
		else uniMat("transform", rect(pos.x * SCALE_UP_FACTOR, pos.y * SCALE_UP_FACTOR, 32 * SCALE_UP_FACTOR, 32 * SCALE_UP_FACTOR));
		if (abs(vel.x) <= 1)
			glBindTexture(GL_TEXTURE_2D, still);
		else glBindTexture(GL_TEXTURE_2D, run);
	//}
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Player::respawn(glm::vec2 spawnpoint) {
	//((World*)_getPointer("world"))->loadWorld(((World*)_getPointer("world"))->currentWorld);
	//((World*)_getPointer("world"))->loadNext = true; // Prevents world from loading next level
	pos = spawnpoint;
	collisions = glm::ivec4(0);
	frictionOffsetting = 0;
	vel = glm::vec2(0);
}