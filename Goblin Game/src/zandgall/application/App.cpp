#define NBT_COMPILE
#include "App.h"
#include "../base/glhelper.h"
#include "../appwork/appwork.h"
#include "../base/networker.h"
#include "World.h"
#include "Player.h"
#include "Assets.h"

#define timestamp() (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count())

App* App::instance = nullptr;
app::window* the;

const bool AABB(glm::vec4 a, glm::vec4 b) { return (a.x + a.z >= b.x && a.x <= b.x + b.z) && (a.y + a.w >= b.y && a.y <= b.y + b.w); }

app::button* createServer, * connectServer;
app::textinput* serverAddress;
app::text* ipIndicator;
bool IS_SERVER = false, IS_CONNECTED = false;

World world;
Player player;
app::component* canvas;

std::vector<Player*> players = std::vector<Player*>();

struct Camera {
	glm::vec2 target = glm::vec2(0);
	glm::vec2 position = glm::vec2(0);
	float zoom = 1;
	float target_zoom = 1;
	float catchup_ratio = 0.1;
} camera;

unsigned tile_layer_framebuffer = 0;
struct Layer {
	unsigned texture, shadow_map;
	int layer;
	std::vector<size_t> objects = std::vector<size_t>();
};

std::map<int, Layer> tile_layers = std::map<int, Layer>();

double ping_start = 0;

void client_handler(SOCKET sock, char* bytes, size_t size) {
	switch (bytes[0]) {
	case message_types::connect_message: {
		if (players.size() > 1)
			return;
		ping_start = glfwGetTime();
		//client::send_message(new char[1]{ message_types::ping }, 1);
		players.push_back(new Player());
		players[1]->object_id = world.objects.size();
		int i = world.objects.size();
		world.objects.push_back(Object());
		world.objects[i].pos = glm::vec2(220, 40);
		world.objects[i].data << new nbt::c("vel");
		world.objects[i].data["vel"].c() << new nbt::ft("x", 0);
		world.objects[i].data["vel"].c() << new nbt::ft("y", 0);
		std::cout << "Successfully connected!" << std::endl;
		break;
	}
	case message_types::player_state: {
		if (bytes[1] > 1) {
			std::cout << "Received player data for " << bytes[1] << std::endl;
			return;
		}
		float x, y, xvel, yvel;
		nbt::fromBytes(&bytes[2], &x);
		nbt::fromBytes(&bytes[6], &y);
		nbt::fromBytes(&bytes[10], &xvel);
		nbt::fromBytes(&bytes[14], &yvel);
		players[bytes[1]]->pos = glm::vec2(x, y);
		players[bytes[1]]->vel = glm::vec2(xvel, yvel);
		players[bytes[1]]->control = glm::bvec4(bytes[18] & 0x1, bytes[18] & 0x2, bytes[18] & 0x4, bytes[18] & 0x8);
		break;
	}
	case message_types::pong: {
		//std::cout << "Ping: " << (glfwGetTime() - ping_start) * 1000 << "ms" << std::endl;
		break;
	}
	case message_types::object_update: {
		float x, y;
		nbt::fromBytes(&bytes[2], &x);
		nbt::fromBytes(&bytes[6], &y);
		world.objects[bytes[1]].pos = glm::vec2(x, y);
		world.objects[bytes[1]].data.load(bytes, 10);
		break;
	}
	}
}

void client_button(app::button* button) {
	std::string text = serverAddress->display->string;
	if (text.size() == 0) {
		std::cout << "Connecting to localhost";
		client::connect("localhost", "32323");
	} else if (text.find(":") != std::string::npos) {
		std::cout << "Connecting to ip " << text.substr(0, text.find(":")) << " port " << text.substr(text.find(":") + 1) << std::endl;
		client::connect(text.substr(0, text.find(":")), text.substr(text.find(":") + 1));
	}
	else {
		std::cout << "Connecting to ip " << text << " port 32323" << std::endl;
		client::connect(text, "32323");
	}
	client::on_message = &client_handler;
	std::cout << "Connected to server! localhost:32323 " << WSAGetLastError() << std::endl;
	the->components.erase(std::find(the->components.begin(), the->components.end(), createServer));
	the->components.erase(std::find(the->components.begin(), the->components.end(), connectServer));
	the->components.erase(std::find(the->components.begin(), the->components.end(), serverAddress));
	the->components.erase(std::find(the->components.begin(), the->components.end(), ipIndicator));
	IS_CONNECTED = true;
}

void server_handler(SOCKET sock, char* bytes, size_t size) {
	switch (bytes[0]) {
	case message_types::player_input:
		players[1]->control = glm::bvec4(bytes[1] & 0x1, bytes[1] & 0x2, bytes[1] & 0x4, bytes[1] & 0x8);
		break;
	case message_types::player_flag:
		players[1]->flags = bytes[1];
		break;
	//case message_types::ping:
	//	std::cout << "Received ping!! Responding..." << std::endl;
	//	server::send_to(sock, new char[1]{ message_types::pong }, 1);
	}
}

void connection_handler(SOCKET sock) {
	players.push_back(new Player());
	players[1]->pos = world.spawn;
	int i = world.objects.size();
	players[1]->object_id = i;
	world.objects.push_back(Object());
	world.objects[i].pos = glm::vec2(220, 40);
	world.objects[i].data << new nbt::c("vel");
	world.objects[i].data["vel"].c() << new nbt::ft("x", 0);
	world.objects[i].data["vel"].c() << new nbt::ft("y", 0);
	std::cout << "Connecting new client" << std::endl;
	server::send_to(sock, new char[1]{message_types::connect_message}, 1);
}

void server_button(app::button* button) {
	server::connect("32323");
	server::on_message = &server_handler;
	server::on_connect = &connection_handler;
	std::cout << "Started server on port 32323 " << WSAGetLastError() << std::endl;
	//client_button(button); // Lets not have a client for this!
	the->components.erase(std::find(the->components.begin(), the->components.end(), createServer));
	the->components.erase(std::find(the->components.begin(), the->components.end(), connectServer));
	the->components.erase(std::find(the->components.begin(), the->components.end(), serverAddress));
	the->components.erase(std::find(the->components.begin(), the->components.end(), ipIndicator));
	IS_SERVER = true, IS_CONNECTED = true;
}

void object_gravity_collision_function(Object* obj, float delta, float nextDelta, World& world, bool interact_with_other_objects, bool same_type) {
	glm::ivec4 col = glm::ivec4(0);
	if (obj->data.has("collisions")) {
		col.x = obj->data["collisions"]["left"].i();
		col.y = obj->data["collisions"]["up"].i();
		col.z = obj->data["collisions"]["right"].i();
		col.w = obj->data["collisions"]["down"].i();
	}
	col -= glm::ivec4(1);
	col.x = max(col.x, 0);
	col.y = max(col.y, 0);
	col.z = max(col.z, 0);
	col.w = max(col.w, 0);

	glm::vec2 vel = glm::vec2(0);
	if (obj->data.has("vel")) {
		vel = glm::vec2(obj->data["vel"]["x"].f(), obj->data["vel"]["y"].f());
	}
	glm::vec2& pos = obj->pos;
	glm::vec2& size = world.object_definitions[obj->object].size;
	if(interact_with_other_objects)
		for (int i = 0; i < world.objects.size(); i++) {
			if (&world.objects[i] == obj || (same_type && world.object_definitions[world.objects[i].object].type != world.object_definitions[obj->object].type))
				continue;
			Object& o = world.objects[i];
			if (!world.object_definitions[o.object].solid && !(same_type && world.object_definitions[world.objects[i].object].type == world.object_definitions[obj->object].type))
				continue;
			glm::vec2 v = glm::vec2(0);
			if (o.data.has("vel"))
				v = glm::vec2(o.data["vel"]["x"].f(), o.data["vel"]["y"].f());
			if (vel.y >= v.y && AABB(glm::vec4(o.pos.x + v.x * delta, o.pos.y + v.y * delta, world.object_definitions[o.object].size), glm::vec4(pos.x + vel.x * delta, pos.y + vel.y * delta + size.y + 1, size.x, vel.y * delta))) {
				pos.y = o.pos.y - size.y;
				col.w = 5;
				vel.y = v.y;
				if (o.data.has("collisions"))
					o.data["collisions"]["up"].i() = 2;
			}

			if (v.y >= vel.y && AABB(glm::vec4(o.pos.x + v.x * delta + 1, o.pos.y + world.object_definitions[o.object].size.y + 1, world.object_definitions[o.object].size.x - 2, v.y * delta), glm::vec4(pos + vel * delta, size))) {
				o.pos.y = pos.y - world.object_definitions[o.object].size.y;
				v.y = vel.y;
				if (o.data.has("collisions"))
					o.data["collisions"]["down"].i() = 5;
			}

			if (vel.x >= v.x && AABB(glm::vec4(o.pos + v * delta + glm::vec2(0, 3), world.object_definitions[o.object].size.x, world.object_definitions[o.object].size.y), glm::vec4(pos + vel * delta, size))) {
				pos.x = o.pos.x - size.x;
				col.z = 1;
				vel.x = v.x;
				if (o.data.has("collisions"))
					o.data["collisions"]["left"].i() = 2;
			}
			if (vel.x <= v.x && AABB(glm::vec4(o.pos + v * delta, world.object_definitions[o.object].size), glm::vec4(pos + vel * delta, size))) {
				pos.x = o.pos.x + world.object_definitions[o.object].size.x;
				col.x = 1;
				vel.x = v.x;
				if (o.data.has("collisions"))
					o.data["collisions"]["right"].i() = 2;
			}
		}

	if (col.w < 5)
		vel.y += 50;
	vel -= vel * min(delta * 10, 1);
	if (abs(vel.x) < 0.1)
		vel.x = 0;
	if (abs(vel.y) < 0.1)
		vel.y = 0;

	pos += vel * delta;
	if (obj->data.has("flip")) {
		if (vel.x > 0)
			obj->data["flip"].b() = true;
		else if (vel.x < 0)
			obj->data["flip"].b() = false;
	}

	if (vel.y >= 0 && (world.solid(pos.x + 1 - vel.x * nextDelta, pos.y + size.y) || world.solid(pos.x + size.x - 1 - vel.x * nextDelta, pos.y + size.y))) {
		pos.y = floor((pos.y + size.y) / TILE_SIZE) * TILE_SIZE - size.y;
		col.w = 5;
		vel.y = 0;
	}
	if (vel.x >= 0 && (world.solid(pos.x + size.x, pos.y + 1 - vel.y * nextDelta) || world.solid(pos.x + size.x, pos.y + size.y - 1 - vel.y * nextDelta))) {
		pos.x = floor((pos.x + size.x) / TILE_SIZE) * TILE_SIZE - size.x;
		col.z = 1;
		vel.x = 0;
	}
	if (vel.x <= 0 && (world.solid(pos.x, pos.y + 1 - vel.y * nextDelta) || world.solid(pos.x, pos.y + size.y - 1 - vel.y * nextDelta))) {
		pos.x = floor((pos.x) / TILE_SIZE) * TILE_SIZE + TILE_SIZE;
		col.x = 1;
		vel.x = 0;
	}
	if (vel.y <= 0 && (world.solid(pos.x + 1, pos.y) || world.solid(pos.x + size.x - 1, pos.y))) {
		pos.y = floor((pos.y) / TILE_SIZE) * TILE_SIZE + TILE_SIZE;
		col.y = 1;
		vel.y = 0;
	}

	if (!obj->data.has("collisions")) {
		obj->data << new nbt::c("collisions");
		obj->data["collisions"].c() << new nbt::it("left", 0);
		obj->data["collisions"].c() << new nbt::it("up", 0);
		obj->data["collisions"].c() << new nbt::it("right", 0);
		obj->data["collisions"].c() << new nbt::it("down", 0);
	}
	obj->data["collisions"]["left"].i() = col.x;
	obj->data["collisions"]["up"].i() = col.y;
	obj->data["collisions"]["right"].i() = col.z;
	obj->data["collisions"]["down"].i() = col.w;
	if (!obj->data.has("vel")) {
		obj->data << new nbt::c("vel");
		obj->data["vel"].c() << new nbt::ft("x", 0);
		obj->data["vel"].c() << new nbt::ft("y", 0);
	}
	obj->data["vel"]["x"].f() = vel.x;
	obj->data["vel"]["y"].f() = vel.y;
}

void object_gravity_collision_func_no_objects(Object* obj, float delta, float nextDelta, World& world) { object_gravity_collision_function(obj, delta, nextDelta, world, false, false); }
void object_gravity_collision_func_objects(Object* obj, float delta, float nextDelta, World& world) { object_gravity_collision_function(obj, delta, nextDelta, world, true, false); }

void worm_update_func(Object* worm, float delta, float nextDelta, World& world) {
	if (!worm->data.has("vel")) {
		worm->pos.x += FRAND;
		worm->data << new nbt::c("vel");
		worm->data["vel"].c() << new nbt::ft("x", 1);
		worm->data["vel"].c() << new nbt::ft("y", 0);
	}
	object_gravity_collision_function(worm, delta, nextDelta, world, true, true);
	if (worm->data["collisions"]["left"].i())
		worm->data["vel"]["x"].f() = 1;
	if (worm->data["collisions"]["right"].i())
		worm->data["vel"]["x"].f() = -1;

	if (worm->data["vel"]["x"].f() >= 0)
		worm->data["vel"]["x"].f() = 1;
	else
		worm->data["vel"]["x"].f() = -1;
}

void worm_render_func(Object* worm, World& world) {
	uniMat("uvtransform", glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, floor(fmod(worm->pos.x, 1.0)*3.0)*0.25, 0)), glm::vec3(1, 0.25, 1)));
	uniMat("transform", rect(floor(worm->pos.x) * SCALE_UP_FACTOR, worm->pos.y * SCALE_UP_FACTOR, 5 * SCALE_UP_FACTOR, 2 * SCALE_UP_FACTOR));
	glBindTexture(GL_TEXTURE_2D, worm_sheet);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	uniMat("uvtransform", glm::mat4(1));
}

void draw(app::component*);

nbt::compound message = nbt::compound();

void send_message(char* bytes, size_t size) {
	if (IS_SERVER)
		server::send_all(bytes, size);
	else if(IS_CONNECTED)
		client::send_message(bytes, size);
}

App::App() {
	_addGLShader("world", loadShader("res/shaders/world.shader"));
	_addGLShader("shadow blur", loadShader("res/shaders/shadow_blur.shader"));

	instance = this;
	init_networking();
	loadAssets();
	
	the = app::create_window("Window", SCREEN_WIDTH * 3, SCREEN_HEIGHT * 3);
	this->window = the;
	the->clear_color = glm::vec4(0.2, 0.2, 0.3 , 1);
	object_type_update.push_back(nullptr);
	object_type_update.push_back(nullptr);
	object_type_update.push_back(nullptr);
	object_type_update.push_back(&worm_update_func);
	object_type_render.push_back(nullptr);
	object_type_render.push_back(nullptr);
	object_type_render.push_back(nullptr);
	object_type_render.push_back(&worm_render_func);


	world = World();
	world.objects.push_back(Object());
	world.objects[0].pos = glm::vec2(220, 40);
	world.objects[0].data << new nbt::c("vel");
	world.objects[0].data["vel"].c() << new nbt::ft("x", 0);
	world.objects[0].data["vel"].c() << new nbt::ft("y", 0);
	world.loadWorld("res/worlds/level3.nbt");

	for (int i = 0; i < world.tile_definitions.size(); i++)
		tile_layers[world.tile_definitions[i].layer] = Layer(0, 0, world.tile_definitions[i].layer);

	for (int i = 0; i < world.objects.size(); i++) {
		if (world.objects[i].object == 0)
			continue;
		if (tile_layers.find(world.object_definitions[world.objects[i].object].layer) == tile_layers.end())
			tile_layers[world.object_definitions[world.objects[i].object].layer] = Layer(0, 0, world.object_definitions[world.objects[i].object].layer);
		tile_layers[world.object_definitions[world.objects[i].object].layer].objects.push_back(i);
	}

	glGenFramebuffers(1, &tile_layer_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, tile_layer_framebuffer);
	glUseProgram(_getGLShader("default shader"));
	glBindVertexArray(the->globjects["Square VAO"]);
	for (auto layer = tile_layers.begin(); layer != tile_layers.end(); layer++) {
		glGenTextures(1, &layer->second.texture);
		glBindTexture(GL_TEXTURE_2D, layer->second.texture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, world.width * TILE_SIZE, world.height * TILE_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glGenTextures(1, &layer->second.shadow_map);
		glBindTexture(GL_TEXTURE_2D, layer->second.shadow_map);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, world.width * TILE_SIZE, world.height * TILE_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, layer->second.texture, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, layer->second.shadow_map, 0);

		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glViewport(0, 0, world.width * TILE_SIZE, world.height * TILE_SIZE);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);

		glDrawBuffer(GL_COLOR_ATTACHMENT1);
		glViewport(0, 0, world.width * TILE_SIZE, world.height * TILE_SIZE);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);


		for (int i = 0; i < world.width; i++) {
			for (int j = 0; j < world.height; j++) {
				if (world.tiles[j][i] == -1 || world.tile_definitions[world.tiles[j][i]].layer != layer->first)
					continue;
				glUseProgram(_getGLShader("default shader"));
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
				glViewport(0, 0, world.width * TILE_SIZE, world.height * TILE_SIZE);
				uniMat("screenspace", glm::ortho<float>(0, world.width * TILE_SIZE, 0, world.height * TILE_SIZE));
				uniMat("transform", rect(i * TILE_SIZE, j * TILE_SIZE, TILE_SIZE, TILE_SIZE));
				uniVec("col", glm::vec4(1));
				uniBool("useTex", true);
				glBindTexture(GL_TEXTURE_2D, world.tile_definitions[world.tiles[j][i]].texture);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

				glUseProgram(_getGLShader("shadow blur"));
				glDrawBuffer(GL_COLOR_ATTACHMENT1);
				uniMat("screenspace", glm::ortho<float>(0, world.width * TILE_SIZE, 0, world.height * TILE_SIZE));
				uniMat("transform", rect(i * TILE_SIZE, j * TILE_SIZE, TILE_SIZE, TILE_SIZE));
				uniVec("col", glm::vec4(1));
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
		}
		glBindTexture(GL_TEXTURE_2D, layer->second.texture);
		glGenerateMipmap(GL_TEXTURE_2D);
		glUseProgram(_getGLShader("shadow blur"));
		glDrawBuffer(GL_COLOR_ATTACHMENT1);
		glBindTexture(GL_TEXTURE_2D, layer->second.shadow_map);

		for (int i = 0; i < 3; i++) {
			glBindTexture(GL_TEXTURE_2D, layer->second.shadow_map);
			glGenerateMipmap(GL_TEXTURE_2D);
			glViewport(0, 0, world.width * TILE_SIZE, world.height * TILE_SIZE);
			uniMat("screenspace", glm::ortho<float>(0, world.width * TILE_SIZE, 0, world.height * TILE_SIZE));
			uniMat("transform", rect(0, 0, world.width * TILE_SIZE, world.height * TILE_SIZE));
			uniVec("col", glm::vec4(1));
			uniBool("useTex", true);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glDeleteFramebuffers(1, &tile_layer_framebuffer);

	player.object_id = 0;
	player.pos = world.spawn;
	players.push_back(&player);

	canvas = new app::component(0, 0, the->width, the->height);
	the->addComponent(canvas);
	canvas->draw.push_back(&draw);

	createServer = new app::button(10, 10, 300, 30);
	createServer->setDefaultStyle();
	createServer->on_click.push_back(&server_button);
	createServer->addChild(new app::text(0, 0, glm::vec4(1), "Create Server"));
	((app::text*)createServer->children[0])->fitParent(glm::vec4(10, 5, 10, 5));
	the->addComponent(createServer);

	connectServer = new app::button(10, 70, 300, 30);
	connectServer->setDefaultStyle();
	connectServer->on_click.push_back(&client_button);
	connectServer->addChild(new app::text(0, 0, glm::vec4(1), "Connect to Server"));
	((app::text*)connectServer->children[0])->fitParent(glm::vec4(10, 5, 10, 5));
	the->addComponent(connectServer);

	serverAddress = new app::textinput(the, 470, 70, 400, 30);
	serverAddress->background_color = glm::vec4(0.5, 0.5, 0.5, 1);
	the->addComponent(serverAddress);

	ipIndicator = new app::text(320, 75, glm::vec4(0, 0, 0, 1), "Server IP:", 24);
	the->addComponent(ipIndicator);
}

long long last = 0;
void App::tick(double delta) {
	if (glfwWindowShouldClose(the->glfw_window)) {
		glfwSetWindowShouldClose(WINDOW, GLFW_TRUE);
		IS_CONNECTED = false;
		IS_SERVER = false;
		return;
	}
	glm::bvec4 control;
	control.x = the->keys[GLFW_KEY_A] || the->keys[GLFW_KEY_LEFT];
	control.y = the->keys[GLFW_KEY_W] || the->keys[GLFW_KEY_UP];
	control.z = the->keys[GLFW_KEY_D] || the->keys[GLFW_KEY_RIGHT];
	control.w = the->keys[GLFW_KEY_S] || the->keys[GLFW_KEY_DOWN];

	canvas->width = the->width; 
	canvas->height = the->height;
	
	if (the->keys[GLFW_KEY_R]) {
		if (IS_SERVER || !IS_CONNECTED)
			player.flags |= Player::respawn_flag;
		else if (!IS_SERVER && IS_CONNECTED)
			send_message(new char[2]{ message_types::player_flag, Player::respawn_flag }, 2);
	}

	if (IS_SERVER || !IS_CONNECTED) {
		player.control = control;
		for (int i = 0; i < players.size(); i++) {
			players[i]->tick(delta, delta, world, the);
			if (IS_SERVER) {
				char* msg = new char[19];
				msg[0] = player_state;
				// 0 for server is the server's player, but it is the other way around for the client. So, invert i to get the client's index for the current updated player
				msg[1] = 1 - i;
				nbt::toBytes(players[i]->pos.x, &msg[2]);
				nbt::toBytes(players[i]->pos.y, &msg[6]);
				nbt::toBytes(players[i]->vel.x, &msg[10]);
				nbt::toBytes(players[i]->vel.y, &msg[14]);
				msg[18] = char(players[i]->control.x * 0x1 + players[i]->control.y * 0x2 + players[i]->control.z * 0x4 + players[i]->control.w * 0x8);
				server::send_all(msg, 19);
				delete[] msg;
			}
		}
	} else if (!IS_SERVER && IS_CONNECTED) {
		for (int i = players.size() - 1; i >= 0; i--)
			players[i]->tick(delta, delta, world, the);
		//if (long long(glfwGetTime() / 5) != last) {
		//	ping_start = glfwGetTime();
		//	client::send_message(new char[1]{ message_types::ping }, 1);
		//	last = glfwGetTime() / 5;
		//}
	}
	send_message(new char[2]{ message_types::player_input, char(control.x * 0x1 + control.y * 0x2 + control.z * 0x4 + control.w * 0x8) }, 2);
	if (IS_SERVER || !IS_CONNECTED)
		for (int i = 0; i < world.objects.size(); i++) {
			if (object_type_update[world.object_definitions[world.objects[i].object].type]) {
				object_type_update[world.object_definitions[world.objects[i].object].type](&world.objects[i], delta, delta, world);
				if (IS_SERVER) {
					std::vector<char> msg = std::vector<char>(10);
					msg[0] = message_types::object_update;
					msg[1] = i;
					nbt::toBytes(world.objects[i].pos.x, &msg[2]);
					nbt::toBytes(world.objects[i].pos.y, &msg[6]);
					world.objects[i].data.write(msg);
					server::send_all(&msg[0], msg.size());
				}
			}
		}

	//if (the->mouseMiddle) {
	//	camera.position += glm::vec2(the->mouseX - the->pmouseX, the->mouseY - the->pmouseY);
	//}
	camera.target_zoom += the->mouseScroll * camera.target_zoom * 0.2;
	camera.target_zoom = max(float(SCREEN_HEIGHT)/float(world.height*TILE_SIZE), max(float(SCREEN_WIDTH) / float(world.width * TILE_SIZE), camera.target_zoom));
	camera.zoom = (1 - camera.catchup_ratio) * camera.zoom + camera.catchup_ratio * camera.target_zoom;
	camera.zoom = max(float(SCREEN_HEIGHT) / float(world.height * TILE_SIZE), max(float(SCREEN_WIDTH) / float(world.width * TILE_SIZE), camera.zoom));

	camera.target = player.pos + glm::vec2(16);
	camera.target.x = min(world.width * TILE_SIZE - SCREEN_WIDTH * 0.5 - SCREEN_WIDTH * 0.5 / camera.zoom, max(SCREEN_WIDTH * 0.5 / camera.zoom - SCREEN_WIDTH * 0.5, camera.target.x - SCREEN_WIDTH * 0.5)) + SCREEN_WIDTH*0.5;
	camera.target.y = min(world.height * TILE_SIZE - SCREEN_HEIGHT * 0.5 - SCREEN_HEIGHT * 0.5 / camera.zoom, max(SCREEN_HEIGHT * 0.5 / camera.zoom - SCREEN_HEIGHT * 0.5, camera.target.y - SCREEN_HEIGHT * 0.5)) + SCREEN_HEIGHT*0.5;
	camera.position = (1-camera.catchup_ratio) * camera.position + camera.catchup_ratio * (camera.target - glm::vec2(SCREEN_WIDTH*0.5, SCREEN_HEIGHT*0.5));
	camera.position.x = min(world.width * TILE_SIZE - SCREEN_WIDTH * 0.5 - SCREEN_WIDTH * 0.5 / camera.zoom, max(SCREEN_WIDTH * 0.5 / camera.zoom - SCREEN_WIDTH * 0.5, camera.position.x));
	camera.position.y = min(world.height * TILE_SIZE - SCREEN_HEIGHT * 0.5 - SCREEN_HEIGHT * 0.5 / camera.zoom, max(SCREEN_HEIGHT * 0.5 / camera.zoom - SCREEN_HEIGHT * 0.5, camera.position.y));
	//}
}

void App::render() {

}

void draw(app::component* component) {
	glUseProgram(_getGLShader("world"));
	uniMat("camera", glm::translate(glm::scale(glm::translate(glm::mat4(1), glm::vec3(SCREEN_WIDTH * SCALE_UP_FACTOR * 0.5, SCREEN_HEIGHT * SCALE_UP_FACTOR * 0.5, 0)), glm::vec3(camera.zoom)), glm::vec3(-SCREEN_WIDTH * SCALE_UP_FACTOR * 0.5 - camera.position.x * SCALE_UP_FACTOR, -SCREEN_HEIGHT * SCALE_UP_FACTOR * 0.5 - camera.position.y * SCALE_UP_FACTOR, 0)));
	uniMat("uvtransform", glm::mat4(1));
	glActiveTexture(GL_TEXTURE0);
	uniVec("col", glm::vec4(1));
	uniBool("useTex", true);
	for (auto layer = tile_layers.begin(); layer != tile_layers.end(); layer++) {
		uniMat("transform", rect(0, 0, world.width * TILE_SIZE * SCALE_UP_FACTOR, world.height * TILE_SIZE * SCALE_UP_FACTOR));
		uniVec("col", glm::vec4(0, 0, 0.1, 0.5));
		glBindTexture(GL_TEXTURE_2D, layer->second.shadow_map);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		uniVec("col", glm::vec4(1)); 
		glBindTexture(GL_TEXTURE_2D, layer->second.texture);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		for (int i = 0; i < layer->second.objects.size(); i++) {
			Object& obj = world.objects[layer->second.objects[i]];
			ObjectDefinition& def = world.object_definitions[obj.object];
			if (object_type_render[def.type] != nullptr) {
				object_type_render[def.type](&world.objects[layer->second.objects[i]], world);
			} else {
				uniMat("transform", rect(obj.pos.x * SCALE_UP_FACTOR, obj.pos.y * SCALE_UP_FACTOR, def.size.x * SCALE_UP_FACTOR, def.size.y * SCALE_UP_FACTOR));
				glBindTexture(GL_TEXTURE_2D, def.texture);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
		}
	}
	
	glUseProgram(_getGLShader("goblin shader"));
	uniMat("camera", glm::translate(glm::scale(glm::translate(glm::mat4(1), glm::vec3(SCREEN_WIDTH * SCALE_UP_FACTOR * 0.5, SCREEN_HEIGHT * SCALE_UP_FACTOR * 0.5, 0)), glm::vec3(camera.zoom)), glm::vec3(-SCREEN_WIDTH * SCALE_UP_FACTOR * 0.5 - camera.position.x * SCALE_UP_FACTOR, -SCREEN_HEIGHT * SCALE_UP_FACTOR * 0.5 - camera.position.y * SCALE_UP_FACTOR, 0)));
	for (int i = 0; i < players.size(); i++) {
		players[i]->render();
	}
	glActiveTexture(GL_TEXTURE0);
	glUseProgram(_getGLShader("default shader"));

}