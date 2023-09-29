#pragma once
#include "App.h"
#include "../base/glhelper.h"
#include <vector>
#include <string>
#include <map>
#define NBT_SHORTHAND 
#include <nbt/nbt.hpp>

#define WORLD_DECOR_NONE 0
#define WORLD_DECOR_CLOUDS 1

#define PLATFORM_DECOR_NONE 0
#define PLATFORM_DECOR_FLOWERS 1
#define PLATFORM_DECOR_STONES 2

enum ObjectTypes {
	NONE, OTHER_PLAYER, DECORATION, WORM
};

struct ObjectDefinition {
	unsigned texture = 0;
	int8_t type = DECORATION;
	int layer = 0;
	nbt::compound data;
	bool solid = false;
	glm::vec2 size = glm::vec2(8, 24);
	ObjectDefinition() {
		data = nbt::compound("data");
	}
};

struct Object {
	glm::vec2 pos;
	size_t object;
	float zlayer = 0;
	nbt::compound data;
	Object() {
		object = 0;
		pos = glm::vec2(0);
		data = nbt::compound("data");
	}
	Object(glm::vec2 pos, size_t object) {
		this->object = object;
		this->pos = pos;
		data = nbt::compound("data");
	}
};

struct TileDecoration;
struct TileInfo {
	std::string id;
	unsigned int texture;
	int layer = 0;
	bool solid = true;
	int width = 16;
	int height = 16;
	std::vector<TileDecoration*> decoration = std::vector<TileDecoration*>();
};

struct TileDecoration {
	glm::vec2 pos;
	uint8_t type = 0;
	nbt::compound data; 
	//Platform* platform;
	TileDecoration(uint8_t type) : type(type) {
		pos = glm::vec2(0);
		data = nbt::compound("data");
	}
};
class World {
public:
	bool loadNext = false;
	glm::vec2 spawn = glm::vec2(0);
	std::vector<std::vector<int>> tiles = std::vector<std::vector<int>>();
	std::vector<TileInfo> tile_definitions = std::vector<TileInfo>();
	std::vector<ObjectDefinition> object_definitions = std::vector<ObjectDefinition>();
	std::vector<Object> objects = std::vector<Object>();
	std::string currentWorld = "";
	int width = 600;
	int height = 450;
	World() {
	}
	void loadWorld(const std::string path);
	//void sortPlatformsByDepth();
	bool solid(int x, int y) {
		x /= TILE_SIZE;
		y /= TILE_SIZE;
		if (y < 0 || y >= tiles.size())
			return false;
		if (x < 0 || x >= tiles[y].size())
			return true;
		if (tiles[y][x] == -1)
			return false;
		return tile_definitions[tiles[y][x]].solid;
	}
};

typedef void (*ObjectUpdateFunc)(Object*, float, float, World&);
typedef void (*ObjectRenderFunc)(Object*, World&);
extern std::vector<ObjectUpdateFunc> object_type_update;
extern std::vector<ObjectRenderFunc> object_type_render;