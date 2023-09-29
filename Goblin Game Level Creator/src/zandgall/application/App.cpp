#define NBT_COMPILE
#include "App.h"
#include "../base/glhelper.h"
#include "../appwork/appwork.h"
#include "../base/networker.h"
#include "fileopendiag.h"
#define NBT_SHORTHAND
#include <nbt/nbt.hpp>
#include <loadgz/gznbt.h>
#include <unordered_map>
App* App::instance = nullptr;
app::window* the, *palette;

app::button* tile_add_new, * tile_solid_toggle;
app::textinput* tile_id_writer;
app::text* tile_id_indicator;

app::component* canvas;

app::button* width_inc, * width_dec, *height_inc, *height_dec, *layer_inc, *layer_dec, *type_inc, *type_dec;
app::text* width_display, * height_display, *layer_display, *type_display;
int world_width, world_height;

app::button* object_mode, *tile_mode, *info_mode;

#define MAX_TYPES 2
std::string object_types[] = {"Decoration", "Worm"};

void tile_select(app::button*);
struct TileDefinition {
	unsigned int texture = 0;
	std::string id = "tile";
	bool solid = true;
	app::button* selector;
	int width = 16;
	int height = 16;
	int layer = 0;
};
struct ObjectDefinition {
	unsigned int texture = 0;
	std::string id = "tile";
	bool solid = true;
	app::button* selector;
	int width = 16;
	int height = 16;
	int layer = 0;
	int type = 0;
};
struct Object {
	size_t object;
	glm::vec2 position;
};
size_t selected_tile = -1;
size_t selected_object = -1;
float definitions_offset = 0;
std::vector<TileDefinition> tile_definitions = std::vector<TileDefinition>();
std::vector<ObjectDefinition> object_definitions = std::vector<ObjectDefinition>();
glm::vec3 canvas_pan;
float canvas_zoom = 2;
struct ivec2comp {
	bool operator()(const glm::ivec2& a, const glm::ivec2& b) const {
		return a.x==b.x ? a.y < b.y : a.x < b.x;
	}
};
std::map<glm::ivec2, unsigned int, ivec2comp> tiles = std::map<glm::ivec2, unsigned int, ivec2comp>();
std::vector<Object> objects = std::vector<Object>();
void tile_select(app::button* button) {
	int index = (button->y - 40) / 64 + definitions_offset;
	if (object_mode->value) {
		if (selected_tile == index) {
			std::string path = ChooseOpenFile("Pick object image", "*.png");
			if (path._Equal("NONE"))
				return;
			object_definitions[selected_tile].texture = loadTexture(path.c_str(), GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST);
			((app::image*)object_definitions[selected_tile].selector->children[0])->setTexture(object_definitions[selected_tile].texture);
			((app::image*)object_definitions[selected_tile].selector->children[0])->fitParent();
			auto dim = ((app::image*)object_definitions[selected_tile].selector->children[0])->originalSize();
			object_definitions[selected_tile].width = dim.x;
			object_definitions[selected_tile].height = dim.y;
		}
		else {
			selected_tile = index;
			//tile_id_writer->display->string = object_definitions[selected_tile].id;
			tile_solid_toggle->value = object_definitions[selected_tile].solid;
			tile_solid_toggle->background_color = tile_solid_toggle->value ? glm::vec4(0.42, 0.42, 0.42, 1) : glm::vec4(0.5, 0.5, 0.5, 1);
		}
	}
	else {
		if (selected_tile == index) {
			std::string path = ChooseOpenFile("Pick tile image", "*.png");
			if (path._Equal("NONE"))
				return;
			tile_definitions[selected_tile].texture = loadTexture(path.c_str(), GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST);
			((app::image*)tile_definitions[selected_tile].selector->children[0])->setTexture(tile_definitions[selected_tile].texture);
			((app::image*)tile_definitions[selected_tile].selector->children[0])->fitParent();
			auto dim = ((app::image*)tile_definitions[selected_tile].selector->children[0])->originalSize();
			tile_definitions[selected_tile].width = dim.x;
			tile_definitions[selected_tile].height = dim.y;
		}
		else {
			selected_tile = index;
			//tile_id_writer->display->string = tile_definitions[selected_tile].id;
			tile_solid_toggle->value = tile_definitions[selected_tile].solid;
			tile_solid_toggle->background_color = tile_solid_toggle->value ? glm::vec4(0.42, 0.42, 0.42, 1) : glm::vec4(0.5, 0.5, 0.5, 1);
		}
	}
}

void add_tile_def(app::button* button) {
	if (object_mode->value) {
		std::string path = ChooseOpenFile("Pick object image", "*.png");
		if (path._Equal("NONE"))
			return;
		selected_tile = object_definitions.size();
		object_definitions.push_back(ObjectDefinition());
		object_definitions[selected_tile].selector = new app::button(0, button->y, 64, 64);
		object_definitions[selected_tile].selector->addChild(new app::image());
		object_definitions[selected_tile].selector->on_click.push_back(&tile_select);
		object_definitions[selected_tile].texture = loadTexture(path.c_str(), GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST);
		((app::image*)object_definitions[selected_tile].selector->children[0])->setTexture(object_definitions[selected_tile].texture);
		((app::image*)object_definitions[selected_tile].selector->children[0])->fitParent();
		auto dim = ((app::image*)object_definitions[selected_tile].selector->children[0])->originalSize();
		object_definitions[selected_tile].width = dim.x;
		object_definitions[selected_tile].height = dim.y; the->addComponent(object_definitions[selected_tile].selector);
		//tile_id_writer->display->string = object_definitions[selected_tile].id;
		tile_solid_toggle->value = object_definitions[selected_tile].solid;
		tile_solid_toggle->background_color = tile_solid_toggle->value ? glm::vec4(0.42, 0.42, 0.42, 1) : glm::vec4(0.5, 0.5, 0.5, 1);
	}
	else {
		bool multi = the->keys[GLFW_KEY_LEFT_SHIFT];
		std::string path = ChooseOpenFile("Pick tile image", "*.png");
		if (path._Equal("NONE"))
			return;
		if (multi) {
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			unsigned int uncropped = loadTexture(path.c_str(), GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST);
			int w, h;
			glBindTexture(GL_TEXTURE_2D, uncropped);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
			char* data = new char[16 * 16 * 4];
			for (int j = 0; j < h / 16; j++) {
				for (int i = 0; i < w / 16; i++) {
					selected_tile = tile_definitions.size();
					tile_definitions.push_back(TileDefinition());
					tile_definitions[selected_tile].selector = new app::button(0, button->y, 64, 64);
					tile_definitions[selected_tile].selector->addChild(new app::image());
					tile_definitions[selected_tile].selector->on_click.push_back(&tile_select);

					glGetTextureSubImage(uncropped, 0, i*16, j*16, 0, 16, 16, 1, GL_RGBA, GL_UNSIGNED_BYTE, 16*16*4, data);

					glGenTextures(1, &tile_definitions[selected_tile].texture);
					glBindTexture(GL_TEXTURE_2D, tile_definitions[selected_tile].texture);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
					glGenerateMipmap(GL_TEXTURE_2D);

					((app::image*)tile_definitions[selected_tile].selector->children[0])->setTexture(tile_definitions[selected_tile].texture);
					((app::image*)tile_definitions[selected_tile].selector->children[0])->fitParent();
					tile_definitions[selected_tile].width = 16;
					tile_definitions[selected_tile].height = 16;
					the->addComponent(tile_definitions[selected_tile].selector);
					button->y += 64;
				}
			}
			tile_solid_toggle->value = tile_definitions[selected_tile].solid;
			tile_solid_toggle->background_color = tile_solid_toggle->value ? glm::vec4(0.42, 0.42, 0.42, 1) : glm::vec4(0.5, 0.5, 0.5, 1);
		}
		else {
			selected_tile = tile_definitions.size();
			tile_definitions.push_back(TileDefinition());
			tile_definitions[selected_tile].selector = new app::button(0, button->y, 64, 64);
			tile_definitions[selected_tile].selector->addChild(new app::image());
			tile_definitions[selected_tile].selector->on_click.push_back(&tile_select);
			tile_definitions[selected_tile].texture = loadTexture(path.c_str(), GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST);
			((app::image*)tile_definitions[selected_tile].selector->children[0])->setTexture(tile_definitions[selected_tile].texture);
			((app::image*)tile_definitions[selected_tile].selector->children[0])->fitParent();
			auto dim = ((app::image*)tile_definitions[selected_tile].selector->children[0])->originalSize();
			tile_definitions[selected_tile].width = dim.x;
			tile_definitions[selected_tile].height = dim.y;
			the->addComponent(tile_definitions[selected_tile].selector);
			tile_solid_toggle->value = tile_definitions[selected_tile].solid;
			tile_solid_toggle->background_color = tile_solid_toggle->value ? glm::vec4(0.42, 0.42, 0.42, 1) : glm::vec4(0.5, 0.5, 0.5, 1);
			button->y += 64;
		}
	}
}

void tile_solid_toggle_on(app::button* button) {
	if (selected_tile == -1)
		return;
	if (object_mode->value)
		object_definitions[selected_tile].solid = true;
	else
		tile_definitions[selected_tile].solid = true;
}
void tile_solid_toggle_off(app::button* button) {
	if (selected_tile == -1)
		return;
	if (object_mode->value)
		object_definitions[selected_tile].solid = false;
	else
		tile_definitions[selected_tile].solid = false;
}

void canvas_render(app::component*);

void inc_width(app::button* button) { world_width += 1 * (button->window->keys[GLFW_KEY_LEFT_SHIFT] ? 5 : 1) * (button->window->keys[GLFW_KEY_LEFT_CONTROL] ? 5 : 1); }
void dec_width(app::button* button) { world_width -= 1 * (button->window->keys[GLFW_KEY_LEFT_SHIFT] ? 5 : 1) * (button->window->keys[GLFW_KEY_LEFT_CONTROL] ? 5 : 1); }
void inc_height(app::button* button) { world_height += 1 * (button->window->keys[GLFW_KEY_LEFT_SHIFT] ? 5 : 1) * (button->window->keys[GLFW_KEY_LEFT_CONTROL] ? 5 : 1); }
void dec_height(app::button* button) { world_height -= 1 * (button->window->keys[GLFW_KEY_LEFT_SHIFT] ? 5 : 1) * (button->window->keys[GLFW_KEY_LEFT_CONTROL] ? 5 : 1); }

void inc_layer(app::button* button) {
	if (selected_tile == -1)
		return;
	if (object_mode->value)
		object_definitions[selected_tile].layer += 1 * (button->window->keys[GLFW_KEY_LEFT_SHIFT] ? 5 : 1) * (button->window->keys[GLFW_KEY_LEFT_CONTROL] ? 5 : 1);
	else
		tile_definitions[selected_tile].layer += 1 * (button->window->keys[GLFW_KEY_LEFT_SHIFT] ? 5 : 1) * (button->window->keys[GLFW_KEY_LEFT_CONTROL] ? 5 : 1);
}
void dec_layer(app::button* button) {
	if (selected_tile == -1)
		return;
	if (object_mode->value)
		object_definitions[selected_tile].layer -= 1 * (button->window->keys[GLFW_KEY_LEFT_SHIFT] ? 5 : 1) * (button->window->keys[GLFW_KEY_LEFT_CONTROL] ? 5 : 1);
	else
		tile_definitions[selected_tile].layer -= 1 * (button->window->keys[GLFW_KEY_LEFT_SHIFT] ? 5 : 1) * (button->window->keys[GLFW_KEY_LEFT_CONTROL] ? 5 : 1);
}

void inc_type(app::button* button) {
	if (selected_tile == -1)
		return;
	if (object_mode->value)
		object_definitions[selected_tile].type++;
	object_definitions[selected_tile].type = std::min(MAX_TYPES-1, std::max(0, object_definitions[selected_tile].type));
}

void dec_type(app::button* button) {
	if (selected_tile == -1)
		return;
	if (object_mode->value)
		object_definitions[selected_tile].type--;
	object_definitions[selected_tile].type = std::min(MAX_TYPES - 1, std::max(0, object_definitions[selected_tile].type));
}

void object_mode_toggle_on(app::button* button) {
	selected_tile = -1;
	tile_add_new->y = object_definitions.size() * 64 + 40;
	for (auto def : object_definitions)
		def.selector->x = 0;
	for (auto def : tile_definitions)
		def.selector->x = -64;
}

void object_mode_toggle_off(app::button* button) {
	selected_tile = -1;
	tile_add_new->y = tile_definitions.size() * 64 + 40;
	for (auto def : tile_definitions)
		def.selector->x = 0;
	for (auto def : object_definitions)
		def.selector->x = -64;
}

unsigned int spawn_texture = 0;

App::App() {

	spawn_texture = loadTexture("res/spawn.png", GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST);
	objects.push_back(Object());
	objects[0].object = -1;
	objects[0].position = glm::vec2(10);

	instance = this;
	init_networking();
	_addGLShader("canvas", loadShader("res/shaders/canvas.shader"));

	palette = app::create_window("Palette", 720, 720);
	palette->clear_color = glm::vec4(0.7, 0.7, 0.9, 1);
	the = app::create_window("Window");
	the->clear_color = glm::vec4(0.7, 0.7, 0.9, 1);
	tile_add_new = new app::button(0, 40, 64, 64);
	tile_add_new->setDefaultStyle();
	tile_add_new->addChild(new app::text(0, 0, glm::vec4(1), "+"));
	((app::text*)tile_add_new->children[0])->fitParent(glm::vec4(10, 5, 10, 5));
	tile_add_new->on_click.push_back(&add_tile_def);
	the->addComponent(tile_add_new);

	app::component* packer = new app::component(0, 0, 50, 50, glm::vec4(0.5, 0.5, 0.5, 1));
	//tile_id_indicator = new app::text(5, 0, glm::vec4(0, 0, 0, 1), "ID:");
	//packer->addChild(tile_id_indicator);
	//tile_id_indicator->fitParent(glm::vec4(5));
	//the->addComponent(packer);
	//tile_id_writer = new app::textinput(the, 50, 0, 145, 50);
	//tile_id_writer->background_color = glm::vec4(0.5, 0.5, 0.5, 1);
	//the->addComponent(tile_id_writer);


	tile_solid_toggle = new app::button(0, 0, 64, 20, true);
	tile_solid_toggle->addChild(new app::text(0, 0, glm::vec4(0, 0, 0, 1), "Solid"));
	((app::text*)tile_solid_toggle->children[0])->fitParent();
	tile_solid_toggle->on_toggled_on.push_back(&tile_solid_toggle_on);
	tile_solid_toggle->on_toggled_off.push_back(&tile_solid_toggle_off);
	tile_solid_toggle->setDefaultStyle();
	the->addComponent(tile_solid_toggle);
	

	layer_dec = new app::button(0, 20, 20, 20, false, glm::vec4(0));
	layer_dec->on_release.push_back(&dec_layer);
	layer_dec->addChild(new app::text(0, 0, glm::vec4(0, 0, 0, 1), "-"));
	((app::text*)layer_dec->children[0])->fitParent();
	the->addComponent(layer_dec);
	layer_inc = new app::button(44, 20, 20, 20, false, glm::vec4(0));
	layer_inc->on_release.push_back(&inc_layer);
	layer_inc->addChild(new app::text(0, 0, glm::vec4(0, 0, 0, 1), "+"));
	((app::text*)layer_inc->children[0])->fitParent();
	the->addComponent(layer_inc);

	packer = new app::component(20, 20, 24, 20);
	layer_display = new app::text(0, 0, glm::vec4(0, 0, 0, 1), "0");
	packer->addChild(layer_display);
	layer_display->fitParent();
	the->addComponent(packer);

	type_dec = new app::button(464, the->height - 36, 36, 36, false, glm::vec4(0));
	type_dec->on_release.push_back(&dec_type);
	type_dec->addChild(new app::text(0, 0, glm::vec4(0, 0, 0, 1), "-"));
	((app::text*)type_dec->children[0])->fitParent();
	the->addComponent(type_dec);
	type_inc = new app::button(600, the->height - 36, 36, 36, false, glm::vec4(0));
	type_inc->on_release.push_back(&inc_type);
	type_inc->addChild(new app::text(0, 0, glm::vec4(0, 0, 0, 1), "+"));
	((app::text*)type_inc->children[0])->fitParent();
	the->addComponent(type_inc);

	packer = new app::component(500, the->height - 36, 100, 36);
	type_display = new app::text(0, 0, glm::vec4(0, 0, 0, 1), object_types[0]);
	packer->addChild(type_display);
	type_display->fitParent();
	the->addComponent(packer);

	canvas = new app::component(64, 0, the->width-64, the->height - 36, glm::vec4(0.5));
	canvas_pan = glm::vec3((the->width-64) / 2, (the->height - 36) / 2, 0);
	canvas->draw.push_back(&canvas_render);
	the->addComponent(canvas);

	world_width = 30;
	world_height = 15;
	width_dec = new app::button(64, the->height-36, 36, 36, false, glm::vec4(0));
	width_dec->on_release.push_back(&dec_width);
	width_dec->addChild(new app::text(0, 0, glm::vec4(0, 0,0,1), "-"));
	((app::text*)width_dec->children[0])->fitParent(glm::vec4(2));
	the->addComponent(width_dec);
	width_inc = new app::button(200, the->height - 36, 36, 36, false, glm::vec4(0));
	width_inc->on_release.push_back(&inc_width);
	width_inc->addChild(new app::text(0, 0, glm::vec4(0, 0, 0, 1), "+"));
	((app::text*)width_inc->children[0])->fitParent(glm::vec4(2));
	the->addComponent(width_inc);
	height_dec = new app::button(264, the->height - 36, 36, 36, false, glm::vec4(0));
	height_dec->on_release.push_back(&dec_height);
	height_dec->addChild(new app::text(0, 0, glm::vec4(0, 0, 0, 1), "-"));
	((app::text*)height_dec->children[0])->fitParent(glm::vec4(2));
	the->addComponent(height_dec);
	height_inc = new app::button(400, the->height - 36, 36, 36, false, glm::vec4(0));
	height_inc->on_release.push_back(&inc_height);
	height_inc->addChild(new app::text(0, 0, glm::vec4(0, 0, 0, 1), "+"));
	((app::text*)height_inc->children[0])->fitParent(glm::vec4(2));
	the->addComponent(height_inc);

	packer = new app::component(100, the->height - 36, 100, 36);
	width_display = new app::text(0, 0, glm::vec4(0, 0, 0, 1), "Width: 30");
	packer->addChild(width_display);
	the->addComponent(packer);
	packer = new app::component(300, the->height - 36, 100, 36);
	height_display = new app::text(0, 0, glm::vec4(0, 0, 0, 1), "Height: 15");
	packer->addChild(height_display);
	the->addComponent(packer);


	object_mode = new app::button(10, 10, 100, 36, true);
	object_mode->addChild(new app::text(0, 0, glm::vec4(0, 0, 0, 1), "OBJ Mode"));
	((app::text*)object_mode->children[0])->fitParent(glm::vec4(3));
	object_mode->on_toggled_on.push_back(&object_mode_toggle_on);
	object_mode->on_toggled_off.push_back(&object_mode_toggle_off);
	object_mode->setDefaultStyle();
	object_mode->height = 72;
	palette->addComponent(object_mode);

}

void App::tick(double delta) {
	
	if (glfwWindowShouldClose(the->glfw_window))
		glfwSetWindowShouldClose(WINDOW, GLFW_TRUE);
	if (glfwWindowShouldClose(palette->glfw_window))
		glfwSetWindowShouldClose(WINDOW, GLFW_TRUE);
	
	for (int i = 0; i < tile_definitions.size(); i++) {
		tile_definitions[i].selector->children[0]->x = 0;
		tile_definitions[i].selector->children[0]->y = 0;
		if (i == selected_tile)
			((app::image*)tile_definitions[i].selector->children[0])->fitParent(glm::vec4(2));
		else
			((app::image*)tile_definitions[i].selector->children[0])->fitParent();
	}
	for (int i = 0; i < object_definitions.size(); i++) {
		object_definitions[i].selector->children[0]->x = 0;
		object_definitions[i].selector->children[0]->y = 0;
		if (i == selected_tile)
			((app::image*)object_definitions[i].selector->children[0])->fitParent(glm::vec4(2));
		else
			((app::image*)object_definitions[i].selector->children[0])->fitParent();
	}
	canvas->width = the->width-64;
	canvas->height = the->height - 36;
	width_display->string = "Width: " + std::to_string(world_width);
	width_display->x = 0;
	width_display->y = 0;
	width_display->fitParent();
	height_display->string = "Height: " + std::to_string(world_height);
	height_display->x = 0;
	height_display->y = 0;
	height_display->fitParent();
	if (selected_tile != -1) {
		if (object_mode->value) {
			layer_display->string = std::to_string(object_definitions[selected_tile].layer);
			type_display->string = object_types[object_definitions[selected_tile].type];
			type_display->x = 0;
			type_display->y = 0;
			type_display->fitParent();
		}
		else layer_display->string = std::to_string(tile_definitions[selected_tile].layer);
		layer_display->x = 0;
		layer_display->y = 0;
		layer_display->fitParent();
	}
	
	glm::ivec2 shift = glm::ivec2(0);
	if (the->keys[GLFW_KEY_LEFT] && !the->pKeys[GLFW_KEY_LEFT])
		shift.x -= 1 * (the->keys[GLFW_KEY_LEFT_SHIFT] ? 5 : 1) * (the->keys[GLFW_KEY_LEFT_CONTROL] ? 5 : 1);
	if (the->keys[GLFW_KEY_RIGHT] && !the->pKeys[GLFW_KEY_RIGHT])
		shift.x += 1 * (the->keys[GLFW_KEY_LEFT_SHIFT] ? 5 : 1) * (the->keys[GLFW_KEY_LEFT_CONTROL] ? 5 : 1);
	if (the->keys[GLFW_KEY_UP] && !the->pKeys[GLFW_KEY_UP])
		shift.y -= 1 * (the->keys[GLFW_KEY_LEFT_SHIFT] ? 5 : 1) * (the->keys[GLFW_KEY_LEFT_CONTROL] ? 5 : 1);
	if (the->keys[GLFW_KEY_DOWN] && !the->pKeys[GLFW_KEY_DOWN])
		shift.y += 1 * (the->keys[GLFW_KEY_LEFT_SHIFT] ? 5 : 1) * (the->keys[GLFW_KEY_LEFT_CONTROL] ? 5 : 1);

	if (shift.x != 0 || shift.y != 0) {
		for (int i = 0; i < objects.size(); i++) 
			objects[i].position += shift * glm::ivec2(16);
		std::map<glm::ivec2, unsigned int, ivec2comp> shifted = std::map<glm::ivec2, unsigned int, ivec2comp>();
		while (tiles.size() > 0) {
			auto node = tiles.extract(tiles.begin());
			node.key() += shift;
			shifted.insert(std::move(node));
		}
		tiles = shifted;
	}

	if (the->mouseMiddle)
		canvas_pan += glm::vec3(the->mouseX - the->pmouseX, the->mouseY - the->pmouseY, 0) / canvas_zoom;
	if(the->mouseX > 64)
		canvas_zoom += the->mouseScroll * canvas_zoom * 0.2; 
	else {
		definitions_offset -= the->mouseScroll;
		if (object_mode->value) {
			definitions_offset = std::max(0.f, std::min(object_definitions.size() - 1.f, definitions_offset));
			for (int i = 0; i < object_definitions.size(); i++) {
				if (i < definitions_offset)
					object_definitions[i].selector->x = -64;
				else object_definitions[i].selector->x = 0;
				object_definitions[i].selector->y = 40 + (i - definitions_offset) * 64;
			}
			tile_add_new->y = 40 + (object_definitions.size() - definitions_offset) * 64;
		}
		else {
			definitions_offset = std::max(0.f, std::min(tile_definitions.size() - 1.f, definitions_offset));
			for (int i = 0; i < tile_definitions.size(); i++) {
				if (i < definitions_offset)
					tile_definitions[i].selector->x = -64;
				else tile_definitions[i].selector->x = 0;
				tile_definitions[i].selector->y = 40 + (i - definitions_offset) * 64;
			}
			tile_add_new->y = 40 + (tile_definitions.size()-definitions_offset) * 64;
		}
	}

	if (the->keys[GLFW_KEY_DELETE] && selected_tile!=-1 && (selected_object == -1 || !object_mode->value)) {
		if (object_mode->value) {
			object_definitions.erase(object_definitions.begin() + selected_tile);
			for (int i = 0; i < object_definitions.size(); i++) {
				if (i < definitions_offset)
					object_definitions[i].selector->x = -64;
				else object_definitions[i].selector->x = 0;
				object_definitions[i].selector->y = 40 + (i - definitions_offset) * 64;
			}
			tile_add_new->y = 40 + (object_definitions.size() - definitions_offset) * 64;
		}
		else {
			the->components.erase(std::find(the->components.begin(), the->components.end(), tile_definitions[selected_tile].selector));
			tile_definitions.erase(tile_definitions.begin() + selected_tile);
			for (int i = 0; i < tile_definitions.size(); i++) {
				if (i < definitions_offset)
					tile_definitions[i].selector->x = -64;
				else tile_definitions[i].selector->x = 0;
				tile_definitions[i].selector->y = 40 + (i - definitions_offset) * 64;
			}
			tile_add_new->y = 40 + (tile_definitions.size() - definitions_offset) * 64;
		}
		selected_tile = -1;
	}

	if (object_mode->value) {
		float x = ((the->mouseX - canvas_pan.x - 64) / canvas_zoom + world_width * 8);
		float y = ((the->mouseY - canvas_pan.y) / canvas_zoom + world_height * 8);
		if (the->keys[GLFW_KEY_LEFT_SHIFT]) {
			x = floor(x / 16) * 16;
			y = floor(y / 16) * 16;
		}
		if (the->mouseLeft && !the->pmouseLeft && the->mouseX > 64 && the->mouseY < the->height - 36) {
			int i = 0;
			for (auto obj : objects) {
				if (obj.object == -1 && x >= obj.position.x && y >= obj.position.y && x <= obj.position.x + 32 && y <= obj.position.y + 32) {
					selected_object = i;
					break;
				}
				else if (obj.object != -1 && x >= obj.position.x && y >= obj.position.y && x  <= obj.position.x + object_definitions[obj.object].width && y <= obj.position.y + object_definitions[obj.object].height) {
					selected_object = i;
					break;
				}
				i++;
			}
			if (i == objects.size())
				selected_object = -1;
		}
		if (selected_object != -1 && the->keys[GLFW_KEY_DELETE]) {
			objects.erase(objects.begin() + selected_object);
			selected_object = -1;
		}
		if (the->mouseLeft && the->mouseX > 64 && the->mouseY < the->height - 36 && selected_object != -1) {
			objects[selected_object].position += glm::vec2(the->mouseX - the->pmouseX, the->mouseY - the->pmouseY) / canvas_zoom;
			if (the->keys[GLFW_KEY_LEFT_SHIFT]) {
				objects[selected_object].position = glm::vec2(x, y);
			}
		}
		if (the->mouseLeft && the->mouseX > 64 && the->mouseY < the->height - 36 && selected_object == -1 && selected_tile != -1) {
			selected_object = objects.size();
			objects.push_back(Object(selected_tile, glm::vec2(x, y)));
		}
	}
	else {
		if (the->mouseLeft && the->mouseX > 64 && the->mouseY < the->height - 36 && (selected_tile != -1 || the->keys[GLFW_KEY_LEFT_ALT])) {
			int x = floor(((the->mouseX - canvas->width * 0.5 - 64) / canvas_zoom - canvas_pan.x + canvas->width * 0.5 + world_width * 8) / 16);
			int y = floor(((the->mouseY - canvas->height * 0.5) / canvas_zoom - canvas_pan.y + canvas->height * 0.5 + world_height * 8) / 16);
			if(the->keys[GLFW_KEY_LEFT_ALT] && tiles.find(glm::ivec2(x,y))!=tiles.end())
				selected_tile = tiles[glm::ivec2(x, y)];
			else tiles[glm::ivec2(x, y)] = selected_tile;
		}
		if (the->mouseRight && the->mouseX > 64 && the->mouseY < the->height - 36) {
			int x = floor(((the->mouseX - canvas->width * 0.5 - 64) / canvas_zoom - canvas_pan.x + canvas->width * 0.5 + world_width * 8) / 16); 
			int y = floor(((the->mouseY - canvas->height * 0.5) / canvas_zoom - canvas_pan.y + canvas->height * 0.5 + world_height * 8) / 16);
			tiles[glm::ivec2(x, y)] = -1;
		}
	}

	if (the->keys[GLFW_KEY_LEFT_CONTROL] && the->keys[GLFW_KEY_O]) {

		std::string path = ChooseOpenFile("Get input file", "");
		if (path == "NONE")
			return;

		using namespace nbt;

		c lvl = c();
		std::ifstream input = std::ifstream(path, std::ios::binary);
		input.seekg(0, std::ios::end);
		std::vector<char> inbytes = std::vector<char>(input.tellg());
		input.seekg(0, std::ios::beg);
		input.read(&inbytes[0], inbytes.size());
		std::vector<char> inflated = std::vector<char>();
		nbt::inflate(&inbytes[0], inbytes.size(), &inflated);

		lvl.load(&inflated[0], 0);
		tile_definitions.clear();
		selected_tile = -1;
		for (int i = 0; i < lvl["tile definitions"].li().tags.size(); i++) {
			compound& definition = lvl["tile definitions"][i].c();
			tile_definitions.push_back(TileDefinition());
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
			tile_definitions[i].layer = definition["layer"].i();
			tile_definitions[i].solid = definition["solid"].b();
			tile_definitions[i].selector = new app::button(object_mode->value ? -64 : 0, i * 64 + 40, 64, 64);
			tile_definitions[i].selector->addChild(new app::image());
			tile_definitions[i].selector->on_click.push_back(tile_select);
			((app::image*)tile_definitions[i].selector->children[0])->setTexture(tile_definitions[i].texture);
			((app::image*)tile_definitions[i].selector->children[0])->fitParent();
			the->addComponent(tile_definitions[i].selector);

		}

		tiles.clear();
		world_width = lvl["width"].i();
		world_height = lvl["height"].i();
		for (int i = 0; i < lvl["tiles"].ia().size(); i++) {
			if (lvl["tiles"].ia()[i] != -1)
				tiles[glm::ivec2(i % world_width, i / world_width)] = lvl["tiles"].ia()[i];
		}


		object_definitions.clear();
		if(lvl.has("object definitions"))
			for (int i = 0; i < lvl["object definitions"].li().tags.size(); i++) {
				compound& definition = lvl["object definitions"][i].c();
				object_definitions.push_back(ObjectDefinition());
				glGenTextures(1, &object_definitions[i].texture);
				glBindTexture(GL_TEXTURE_2D, object_definitions[i].texture);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

				if (definition.has("width")) {
					object_definitions[i].width = definition["width"].i();
					object_definitions[i].height = definition["height"].i();
				}
				else {
					object_definitions[i].width = 16;
					object_definitions[i].height = 16;
				}

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, object_definitions[i].width, object_definitions[i].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &definition["texture"].ba()[0]);
				glGenerateMipmap(GL_TEXTURE_2D);
				object_definitions[i].width = definition["width"].i();
				object_definitions[i].height = definition["height"].i();
				object_definitions[i].layer = definition["layer"].i();
				object_definitions[i].type = definition["type"].i() - 2;
				object_definitions[i].solid = definition["solid"].b();
				object_definitions[i].selector = new app::button(object_mode->value ? 0 : -64, i * 64 + 40, 64, 64);
				object_definitions[i].selector->addChild(new app::image());
				object_definitions[i].selector->on_click.push_back(tile_select);
				((app::image*)object_definitions[i].selector->children[0])->setTexture(object_definitions[i].texture);
				((app::image*)object_definitions[i].selector->children[0])->fitParent();
				auto dim = ((app::image*)object_definitions[i].selector->children[0])->originalSize();
				object_definitions[i].width = dim.x;
				object_definitions[i].height = dim.y;
				the->addComponent(object_definitions[i].selector);
			}
		objects.clear();
		objects.push_back(Object(-1, glm::vec2(lvl["spawn"]["x"].f(), lvl["spawn"]["y"].f())));
		if(lvl.has("objects"))
			for (int i = 0; i < lvl["objects"].li().tags.size(); i++) {
				objects.push_back(Object(lvl["objects"][i]["object"].i(), glm::vec2(lvl["objects"][i]["x"].f(), lvl["objects"][i]["y"].f())));
			}
		if (object_mode->value)
			tile_add_new->y = object_definitions.size() * 64 + 40;
		else tile_add_new->y = tile_definitions.size() * 64 + 40;

		lvl.discard();
	}

	if (the->keys[GLFW_KEY_LEFT_CONTROL] && the->keys[GLFW_KEY_S]) {
		using namespace nbt;
		c lvl = c();
		lvl << new c("spawn");
		lvl["spawn"].c() << new ft("x", objects[0].position.x);
		lvl["spawn"].c() << new ft("y", objects[0].position.y);
		li* defs = new li("tile definitions");
		lvl << defs;
		for (int i = 0; i < tile_definitions.size(); i++) {
			defs->add(new c());
			defs->tags[i].c() << new str(tile_definitions[i].id, "id");
			defs->tags[i].c() << new ba("texture");
			int w = 16;
			int h = 16;
			glBindTexture(GL_TEXTURE_2D, tile_definitions[i].texture);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
			defs->tags[i].c() << new it("width", w);
			defs->tags[i].c() << new it("height", h);
			defs->tags[i].c() << new it("layer", tile_definitions[i].layer);
			char* data = new char[w*h*4];
			glGetTextureImage(tile_definitions[i].texture, 0, GL_RGBA, GL_UNSIGNED_BYTE, w * h * 4, data);
			for (int j = 0; j < w * h * 4; j++)
				defs->tags[i]["texture"].ba().push_back(data[j]);
			defs->tags[i].c() << new bt("solid", tile_definitions[i].solid);
		}
		lvl << new ia("tiles");
		for(int y = 0; y < world_height; y++)
			for (int x = 0; x < world_width; x++)
				if (tiles.find(glm::ivec2(x, y)) == tiles.end())
					lvl["tiles"].ia().push_back(-1);
				else lvl["tiles"].ia().push_back(tiles[glm::ivec2(x, y)]);

		defs = new li("object definitions");
		lvl << defs;
		for (int i = 0; i < object_definitions.size(); i++) {
			defs->add(new c());
			defs->tags[i].c() << new str(object_definitions[i].id, "id");
			defs->tags[i].c() << new ba("texture");
			int w = 16;
			int h = 16;
			glBindTexture(GL_TEXTURE_2D, object_definitions[i].texture);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
			defs->tags[i].c() << new it("width", w);
			defs->tags[i].c() << new it("height", h);
			defs->tags[i].c() << new it("type", object_definitions[i].type+2);
			defs->tags[i].c() << new it("layer", object_definitions[i].layer);
			char* data = new char[w * h * 4];
			glGetTextureImage(object_definitions[i].texture, 0, GL_RGBA, GL_UNSIGNED_BYTE, w * h * 4, data);
			for (int j = 0; j < w * h * 4; j++)
				defs->tags[i]["texture"].ba().push_back(data[j]);
			defs->tags[i].c() << new bt("solid", object_definitions[i].solid);
		}
		defs = new li("objects");
		lvl << defs;
		for (int i = 1; i < objects.size(); i++) {
			defs->add(new c());
			defs->tags[i - 1].c() << new it("object", objects[i].object);
			defs->tags[i - 1].c() << new ft("x", objects[i].position.x);
			defs->tags[i - 1].c() << new ft("y", objects[i].position.y);
		}

		lvl << new it("width", world_width);
		lvl << new it("height", world_height);
		std::vector<char> buffer = std::vector<char>();
		lvl.write(buffer);
		std::vector<char> deflated = std::vector<char>();
		nbt::deflate(&buffer[0], buffer.size(), &deflated, 9);
		std::string path = ChooseSaveFile("Save output file", "");
		if (path == "NONE")
			return;
		std::ofstream output = std::ofstream(path, std::ios::binary);
		output.write(&deflated[0], deflated.size());
		lvl.discard();
	}
	else if (the->keys[GLFW_KEY_S] && !the->pKeys[GLFW_KEY_S]) {
		if (object_mode->value && selected_tile != object_definitions.size() - 1)
			tile_select(object_definitions[selected_tile+1].selector);
		else if (!object_mode->value && selected_tile != tile_definitions.size() - 1)
			tile_select(tile_definitions[selected_tile + 1].selector);
	}
	else if (the->keys[GLFW_KEY_W] && !the->pKeys[GLFW_KEY_W]) {
		if (object_mode->value && selected_tile > 0)
			tile_select(object_definitions[selected_tile - 1].selector);
		else if (!object_mode->value && selected_tile > 0)
			tile_select(tile_definitions[selected_tile - 1].selector);
	}
}

void App::render() {
}

void canvas_render(app::component* canvas) {
	glUseProgram(_getGLShader("canvas"));
	glEnable(GL_SCISSOR_TEST);
	glScissor(64, 36, canvas->width, canvas->height);
	uniMat("canvas_space", translation(glm::vec3(64, 0, 0)));
	uniMat("view_space", glm::translate(glm::scale(translation(glm::vec3(canvas->width*0.5, canvas->height*0.5, 0)), glm::vec3(canvas_zoom)), canvas_pan - glm::vec3(canvas->width*0.5, canvas->height*0.5, 0)));
	glActiveTexture(GL_TEXTURE0);


	uniVec("col", glm::vec4(0, 0, 0, 0.5));
	uniBool("useTex", false);
	for (int x = 0; x <= world_width; x++) {
		uniMat("transform", rect(x * 16 - world_width * 8, -world_height*8, 1/canvas_zoom, world_height * 16));
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}
	for (int y = 0; y <= world_height; y++) {
		uniMat("transform", rect(-world_width*8, y * 16 - world_height * 8, world_width*16, 1 / canvas_zoom));
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}

	uniBool("useTex", true);
	for (auto a : tiles) {
		if (a.second == -1)
			continue;
		uniVec("col", glm::vec4(1));
		if(a.first.x >= world_width || a.first.y >= world_height || a.first.x < 0 || a.first.y < 0 || object_mode->value)
			uniVec("col", glm::vec4(1, 1, 1, 0.5));
		uniMat("transform", rect(a.first.x*16-world_width * 8, a.first.y * 16 - world_height * 8, tile_definitions[a.second].width, tile_definitions[a.second].height));
		glBindTexture(GL_TEXTURE_2D, tile_definitions[a.second].texture);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}
	int i = 0;
	for (auto a : objects) {
		uniVec("col", glm::vec4(1));
		if (a.object > object_definitions.size())
			continue;
		if (!object_mode->value || i == selected_object)
			uniVec("col", glm::vec4(1, 1, 1, 0.5));
		if (a.object == -1) {
			uniMat("transform", rect(a.position.x - world_width * 8, a.position.y - world_height * 8, 32, 32));
			glBindTexture(GL_TEXTURE_2D, spawn_texture);
		} else {
			uniMat("transform", rect(a.position.x - world_width * 8, a.position.y - world_height * 8, object_definitions[a.object].width, object_definitions[a.object].height));
			glBindTexture(GL_TEXTURE_2D, object_definitions[a.object].texture);
		}
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		i++;
	}
	if (selected_tile != -1) {
		if (object_mode->value) {
			uniVec("col", glm::vec4(1, 1, 1, 0.5));
			if (the->keys[GLFW_KEY_LEFT_SHIFT])
				uniMat("transform", rect(floor(((the->mouseX - canvas->width * 0.5 - 64) / canvas_zoom - canvas_pan.x + canvas->width * 0.5 + world_width * 8) / 16) * 16 - world_width * 8, floor(((the->mouseY - canvas->height * 0.5) / canvas_zoom - canvas_pan.y + canvas->height * 0.5 + world_height * 8) / 16) * 16 - world_height * 8, object_definitions[selected_tile].width, object_definitions[selected_tile].width));
			else
				uniMat("transform", rect((the->mouseX - canvas->width*0.5 - 64) / canvas_zoom - canvas_pan.x + canvas->width * 0.5, (the->mouseY - canvas->height*0.5) / canvas_zoom - canvas_pan.y + canvas->height*0.5, object_definitions[selected_tile].width, object_definitions[selected_tile].height));
			glBindTexture(GL_TEXTURE_2D, object_definitions[selected_tile].texture);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		} else {
			uniVec("col", glm::vec4(1, 1, 1, 0.5));
			int x = floor(((the->mouseX - canvas->width * 0.5 - 64) / canvas_zoom - canvas_pan.x + canvas->width * 0.5 + world_width * 8) / 16) * 16 - world_width * 8;
			int y = floor(((the->mouseY - canvas->height * 0.5) / canvas_zoom - canvas_pan.y + canvas->height * 0.5 + world_height * 8) / 16) * 16 - world_height * 8;
			uniMat("transform", rect(x, y, tile_definitions[selected_tile].width, tile_definitions[selected_tile].width));
			glBindTexture(GL_TEXTURE_2D, tile_definitions[selected_tile].texture);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
	}
	glDisable(GL_SCISSOR_TEST);

}