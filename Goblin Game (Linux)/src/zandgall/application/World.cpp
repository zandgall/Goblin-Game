#include "World.h"
#include <fstream>
#include <loadgz/gznbt.h>
#include <algorithm>
#include "../base/glhelper.h"

std::vector<ObjectUpdateFunc> object_type_update = std::vector<ObjectUpdateFunc>();
std::vector<ObjectRenderFunc> object_type_render = std::vector<ObjectRenderFunc>();

using namespace nbt;
void World::loadWorld(const std::string path) {
	if (path == "NONE")
		return;
	tiles.clear();
	tile_definitions.clear();

	std::ifstream input(path.c_str(), std::ios::binary);
	input.seekg(0, std::ios::end);
	std::vector<char> inbytes = std::vector<char>(input.tellg());
	input.seekg(0, std::ios::beg);
	input.read(&inbytes[0], inbytes.size());
	
	std::vector<char> obytes = std::vector<char>();
	inflate(&inbytes[0], inbytes.size(), &obytes);

	compound level = compound("level");
	level.load(&obytes[0], 0);

	for (int i = 0; i < level["tile definitions"].li().tags.size(); i++) {
		compound& definition = level["tile definitions"][i].c();
		tile_definitions.push_back(TileInfo{definition["id"].str(), 0, definition["layer"].i(), (bool)definition["solid"].b(), 0, 0, std::vector<TileDecoration*>()});
		glGenTextures(1, &tile_definitions[i].texture);
		glBindTexture(GL_TEXTURE_2D, tile_definitions[i].texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		
		if (definition.has("width")) {
			tile_definitions[i].width = definition["width"].i();
			tile_definitions[i].height = definition["height"].i();
		}
		else {
			tile_definitions[i].width = 16;
			tile_definitions[i].height = 16;
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tile_definitions[i].width, tile_definitions[i].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &definition["texture"].ba()[0]);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	object_definitions.clear();
	object_definitions.push_back(ObjectDefinition());
	object_definitions[0].type = OTHER_PLAYER;
	object_definitions[0].solid = true;
	if (level.has("object definitions"))
		for (int i = 0; i < level["object definitions"].li().tags.size(); i++) {
			compound& definition = level["object definitions"][i].c();
			unsigned texture = 0;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			object_definitions.push_back(ObjectDefinition());
			if (definition.has("width")) {
				object_definitions[i+1].size.x = definition["width"].i();
				object_definitions[i+1].size.y = definition["height"].i();
			}
			else {
				object_definitions[i+1].size.x = 16;
				object_definitions[i+1].size.y = 16;
			}
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, object_definitions[i+1].size.x, object_definitions[i+1].size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, &definition["texture"].ba()[0]);
			glGenerateMipmap(GL_TEXTURE_2D);

			object_definitions[i + 1].texture = texture;
			object_definitions[i + 1].layer = definition["layer"].i();
			object_definitions[i + 1].type = definition["type"].i();
			object_definitions[i + 1].solid = definition["solid"].b();
		
		}

	for (int i = 0; i < objects.size(); i++) {
		if (objects[i].object != 0) {
			objects.erase(objects.begin() + i);
			i--;
		}
	}
	if(level.has("objects"))
		for (int i = 0; i < level["objects"].li().tags.size(); i++)
			objects.push_back(Object(glm::vec2(floor(level["objects"][i]["x"].f()), floor(level["objects"][i]["y"].f())), level["objects"][i]["object"].i()+1));

	width = level["width"].i();
	height = level["height"].i();
	auto ia = level["tiles"].ia();
	for (int i = 0; i < width * height; i++) {
		if (i / width >= tiles.size()) {
			tiles.push_back(std::vector<int>());
		}
		tiles[i / width].push_back(ia[i]); // -1 means no tile
	}

	/*
	list platforms = level["Platforms"].li();
	for (int i = 0; i < platforms.tags.size(); i++) {
		unsigned int platTexture = 0;
		int w, h, ch;
		c pl = platforms[i].c();
		if (pl["image"].value !=nullptr && pl["image"].ba().size() > 0) {
			auto arr = pl["image"].ba();
			glGenTextures(1, &platTexture);
			glBindTexture(GL_TEXTURE_2D, platTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			unsigned char* img = stbi_load_from_memory((const stbi_uc*)&arr[0], arr.size(), &w, &h, &ch, 4);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
		}
		else {
			w = pl["w"].f();
			h = pl["h"].f();
			if (pl["path"].value != nullptr) {
				platTexture = loadTexture(pl["path"].str().c_str());
				glBindTexture(GL_TEXTURE_2D, platTexture);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
			}
		}
		this->platforms.push_back(Platform(glm::vec4(
			pl["x"].f(),
			pl["y"].f(),
			pl["w"].f(),
			pl["h"].f()
		), glm::vec2(pl["ox"].f(), pl["oy"].f()),
			platTexture,
			pl["time"].f()));
		Platform& p = this->platforms[i];
		c prop;
		if (pl["properties"].value == nullptr)
			prop = c();
		else prop = pl["properties"].c();
		p.type = prop["type"].value == nullptr ? 1 : prop["type"].b();
		p.depth = prop["depth"].value == nullptr ? 1.f : prop["depth"].f();
		p.imageWidth = prop["image-width"].value == nullptr ? w : prop["image-width"].i();
		p.imageHeight = prop["image-height"].value == nullptr ? h : prop["image-height"].i();
		p.timeSpeed = prop["time-speed"].value == nullptr ? 1.f : prop["time-speed"].f();
		//this->platforms[i].time *= this->platforms[i].timeSpeed;
		int DECOR_FLAGS = (pl["decoration-flags"].value == nullptr ? INT_MAX : pl["decoration-flags"].i());
		if (DECOR_FLAGS & PLATFORM_DECOR_FLOWERS) {
			for (int j = 0, b = p.decoration.size(), n = rand() % 3 + sqrt(p.size.x * 0.25); j < n; j++, b++) {
					p.decoration.push_back(PlatformDecoration(&this->platforms[i], PLATFORM_DECOR_FLOWERS));
					p.decoration[b].pos.x = FRAND * ((p.size.x-20)/n+1)+(j* (p.size.x - 20) / n) + 10;
					p.decoration[b].data.add(new it("flower", rand() % 3));
					p.decoration[b].data.add(new it("stem", rand() % 3));
					p.decoration[b].data.add(new ft("blow", 0));
			}
		}
		if (DECOR_FLAGS & PLATFORM_DECOR_STONES) {
			for (int j = 0, b = p.decoration.size(), n = rand() % 2 + log10(p.size.x); j < n; j++, b++) {
					p.decoration.push_back(PlatformDecoration(&this->platforms[i], PLATFORM_DECOR_STONES));
					p.decoration[b].pos.x = FRAND * ((p.size.x - 28) / n + 1) + (j * (p.size.x - 20) / n) + 14;
					p.decoration[b].data.add(new it("type", rand() % 4));
					p.decoration[b].data.add(new it("flip", rand() % 2));
			}
		}
	}
	sortPlatformsByDepth();
	*/ /// REUSE FOR OBJECTS

	int DECOR_FLAGS = (level.has("decoration-flags") ? level["decoration-flags"].i() : INT_MAX);

	//if (DECOR_FLAGS & WORLD_DECOR_CLOUDS) {
	//	for (int j = 0, b = decoration.size(), n = rand() % 10 + 10; j < n; j++, b++) {
	//		decoration.push_back(Object(WORLD_DECOR_CLOUDS));
	//		decoration[b].pos.x = FRAND*800;
	//		decoration[b].pos.y = FRAND*200;
	//		decoration[b].data.add(new ft("type", rand() % 10));
	//		decoration[b].data.add(new ft("speed", FRAND*0.1+0.01));
	//	}
	//}

	spawn.x = level["spawn"]["x"].f();
	spawn.y = level["spawn"]["y"].f();
	currentWorld = path;
}

//bool comp(Platform a, Platform b) {
//	return a.depth < b.depth;
//}
//
//void World::sortPlatformsByDepth() {
//	std::sort(platforms.begin(), platforms.end(), comp);
//}