#include "../base/glhelper.h"
#include "../base/handler.h"
#include "../base/Sound.h"
#define NBT_SHORTHAND
#include <nbt/nbt.hpp>

#ifndef APPWORK_H
#define APPWORK_H

#define run_all(functions, param) for (auto a : functions) a(param)

namespace app {

	class window;
	typedef void (*window_function)(window*);
	class component;
	typedef void (*component_function)(component*);
	class button;
	typedef void (*button_function)(button*);
	class slider;
	typedef void (*slider_function)(slider*);

	class component {
	public:
		window* window = nullptr;
		component* parent = nullptr;
		double x = 0, y = 0, width = 0, height = 0;
		//bool overflow_x = true, overflow_y = true;

		unsigned int shader = 0;
		nbt::compound uniforms = nbt::compound();
		glm::vec4 background_color = glm::vec4(0);

		component() { shader = _getGLShader("default shader"); }
		component(component* parent) : parent(parent) {}
		component(double x, double y, double width = 100, double height = 100, glm::vec4 background_color = glm::vec4(0)) : x(x), y(y), width(width), height(height), background_color(background_color) {}

		std::vector<component*> children = std::vector<component*>();
		void addChild(component* child) {
			children.push_back(child);
			child->parent = this;
			child->window = window;
		}
		bool isChildOf(component* potentialParent) {
			if (parent == potentialParent)
				return true;
			else if (parent == nullptr)
				return false;
			else return parent->isChildOf(potentialParent);
		}
		std::vector<component_function> update = std::vector<component_function>();
		std::vector<component_function> draw = std::vector<component_function>();
		glm::vec4 bounds() {
			if (parent)
				return glm::vec4(x, y, 0, 0) + parent->bounds();
			return glm::vec4(x, y, 0, 0);
		}
		void default_update();
		void default_draw() {
			if (this->shader == 0)
				this->shader = _getGLShader("default shader");
			glUseProgram(this->shader);
			uniMat("transform", rect(bounds().x, bounds().y, width, height));
			uniVec("col", background_color);
			uniBool("useTex", false);
			for (auto a : uniforms.tags) {
				switch (a.second->id) {
				case 1:
					uniBool(a.first.c_str(), a.second.b());
					break;
				case 3:
					uniInt(a.first.c_str(), a.second.i());
					break;
				case -3:
					uniUInt(a.first.c_str(), a.second.ui());
					break;
				case 5:
					uniFloat(a.first.c_str(), a.second.f());
					break;
				case 6:
					uniDouble(a.first.c_str(), a.second.d());
					break;
				case 10:
					if (a.second.has("length")) {
						switch (a.second["length"].i()) {
						case 4:
							uniVec(a.first.c_str(), glm::vec4(a.second["x"].f(), a.second["y"].f(), a.second["z"].f(), a.second["w"].f()));
							break;
						case 3:
							uniVec(a.first.c_str(), glm::vec3(a.second["x"].f(), a.second["y"].f(), a.second["z"].f()));
							break;
						case 2:
							uniVec(a.first.c_str(), glm::vec2(a.second["x"].f(), a.second["y"].f()));
							break;
						}
					}
					//else if (a.second.has("width")) {
					//	switch (a.second["width"]) {

					//	}
					//} Matrices?
					else if (a.second.has("texture") && a.second.has("index")) {
						int i = a.second["index"].i();
						uniInt(a.first.c_str(), i);
						glActiveTexture(GL_TEXTURE0 + i);
						glBindTexture(GL_TEXTURE_2D, a.second["texture"].i());
						glActiveTexture(GL_TEXTURE1);
					}
				}
			}
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			run_all(draw, this);
			for (auto a : children)
				a->default_draw();
		}
	};

	
	

	class button : public component {
	public:
		std::vector<button_function> on_click = std::vector<button_function>();
		std::vector<button_function> on_release = std::vector<button_function>();
		std::vector<button_function> on_hover = std::vector<button_function>();
		std::vector<button_function> on_not_hover = std::vector<button_function>();
		std::vector<button_function> on_toggled_on = std::vector<button_function>();
		std::vector<button_function> on_toggled_off = std::vector<button_function>();

		std::vector<button_function> while_mouse_held = std::vector<button_function>();
		std::vector<button_function> while_released = std::vector<button_function>();
		std::vector<button_function> while_hovered = std::vector<button_function>();
		std::vector<button_function> while_not_hovered = std::vector<button_function>();
		std::vector<button_function> while_toggled_on = std::vector<button_function>();
		std::vector<button_function> while_toggled_off = std::vector<button_function>();

		void setDefaultStyle();
		bool toggle = false, value = false, held = false, hovered = false;
		button();
		button(component* parent) : button() { this->parent = parent; }
		button(double x, double y, double width = 100, double height = 100, bool toggle = false, glm::vec4 background_color = glm::vec4(1)) : button() { this->x = x; this->y = y; this->width = width; this->height = height; this->toggle = toggle; this->background_color = background_color; }

	};

	class slider : public component {
	public:
		bool vertical = false;
		double value = 0.5, maximum = 1, minimum = 0, step = 0.1;
		component* thumb;
		slider();
		slider(component* parent) : slider() { this->parent = parent; }
		slider(double x, double y, double value = 0.5, double minimum = 1, double maximum = 0, double step = 0, double width = 100, double height = 100, bool vertical = false, glm::vec4 background_color = glm::vec4(1)) : slider() { this->x = x; this->y = y; this->value = value; this->maximum = maximum; this->minimum = minimum; this->step = step;  this->width = width; this->height = height; this->vertical = vertical; this->background_color = background_color; }
		void setDefaultStyle();
	};

	class draggable : public component {
	public:
		bool sticky = true;
		glm::vec2 offset = glm::vec2(0), origin = glm::vec2(0);
		draggable();
		draggable(component* parent) : draggable() { this->parent = parent; };
		draggable(double x, double y, double width, double height, bool sticky = true, glm::vec4 background_color = glm::vec4(1)) : draggable() { this->x = x; this->y = y; this->width = width; this->height = height; this->sticky = sticky; this->background_color = background_color; }
	};

	class window {
	public:
		GLFWwindow* glfw_window;
		int width = 0, height = 0;
		double pmouseX = 0, pmouseY = 0, pmouseScroll = 0, mouseX = 0, mouseY = 0, mouseScroll = 0;
		bool pmouseLeft = 0, pmouseRight = 0, pmouseMiddle = 0, mouseLeft = 0, mouseRight = 0, mouseMiddle = 0;
		bool* pKeys = new bool[GLFW_KEY_LAST];
		bool* keys = new bool[GLFW_KEY_LAST];
		glm::vec4 clear_color = glm::vec4(1);
		std::map<std::string, unsigned int> globjects = std::map<std::string, unsigned int>();

		std::vector<component*> components = std::vector<component*>();
		component* selected_component = nullptr;
		void addComponent(component* component) {
			components.push_back(component);
			component->parent = nullptr;
			component->window = this;
		}
		std::vector<window_function> update = std::vector<window_function>();
		std::vector<window_function> draw = std::vector<window_function>();
		std::vector<GLFWkeyfun> key_typed = std::vector<GLFWkeyfun>();
		std::vector<GLFWcharfun> char_funcs = std::vector<GLFWcharfun>();
		void default_update() {
			run_all(update, this);
			if (this->mouseLeft && !this->pmouseLeft)
				selected_component = nullptr;
			for (auto a : components)
				a->default_update();
		}
		void default_draw() {
			glfwMakeContextCurrent(glfw_window);
			for (auto a : GL_SHADERS) {
				glUseProgram(a.second);
				uniMat("screenspace", glm::ortho<float>(0, width, height, 0));
			}
			glUseProgram(_getGLShader("default shader"));
			glBindVertexArray(globjects["Square VAO"]);
			glViewport(0, 0, width, height);
			glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT);
			run_all(draw, this);
			for (auto a : components)
				a->default_draw();
			//if (!selected_component)
			//	return;
			//uniVec("col", glm::vec4(1, 0, 1, 0.2)); 
			//uniMat("transform", rect(selected_component->bounds().x, selected_component->bounds().y, selected_component->width, selected_component->height));
			//uniBool("useTex", false);
			//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		window() : glfw_window(nullptr) {}
		window(GLFWwindow* wind) : glfw_window(wind) {}
		void addGLObject(std::string name, unsigned int obj) {
			globjects.insert(std::make_pair(name, obj));
		}
	};
	class text : public component {
	public:
		std::string string = "", font = "roboto";
		int font_size = 48;
		glm::vec4 text_color = glm::vec4(0);
		text();
		text(component* parent) : text() { this->parent = parent;  }
		text(double x, double y, glm::vec4 text_color = glm::vec4(0, 0, 0, 1), std::string string = "", int font_size = 48, std::string font = "roboto") : text() { this->x = x; this->y = y; this->text_color = text_color;  this->string = string, this->font = font; this->font_size = font_size; }
		void fitWidth(double width) {
			glm::vec3 dim = measureString(string, font.c_str());
			if (dim.x == 0)
				dim.x = 1;
			double ratio = dim.z / dim.x;
			this->width = width;
			height = width * ratio;
			font_size = height;
		}
		void fitHeight(double height) {
			glm::vec3 dim = measureString(string, font.c_str());
			double ratio = dim.x / dim.z;
			this->height = height;
			font_size = height;
			width = height * ratio;
		}
		void fit(glm::vec2 bounds, glm::vec4 margin = glm::vec4(0)) {
			glm::vec3 baseSize = measureString(string, font.c_str());
			if ((bounds.x - margin.x - margin.z) / baseSize.x <= (bounds.y - margin.y - margin.w) / baseSize.z) {
				fitWidth(bounds.x - margin.x - margin.z);
				y += (bounds.y - margin.y - margin.w - font_size) / 2.0 + margin.y;
				x += margin.x;
			}
			else {
				fitHeight(bounds.y - margin.y - margin.w);
				x += (bounds.x - width - margin.x - margin.z) / 2.0 + margin.x;
				y += margin.y;
			}
		}
		void fitParent(glm::vec4 margin = glm::vec4(0)) {
			if (!parent) {
				if (!window)
					return;
				fit(glm::vec2(window->width, window->height), margin);
			}
			else
				fit(glm::vec2(parent->width, parent->height), margin);
		}
	};
	class textinput : public component {
	public:
		text* display;
		int marker = 0, select_marker = 0;
		textinput(app::window* win);
		textinput(app::window* win, double x, double y, double width, double height) : textinput(win) { this->x = x; this->y = y; this->width = width; this->height = height; display->fitParent(glm::vec4(5)); display->x = 5; }
	};
	class image: public component {
	public:
		unsigned int texture;
		image();
		image(unsigned int texture) : image() { this->texture = texture; }
		image(std::string path) : image() { setTexture(path); }
		void setLinear() {
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		void setNearest() {
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		void setWrap(unsigned int x, unsigned int y) {
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, x);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, y);
		}
		void setTexture(unsigned int texture) { 
			this->texture = texture; 
			glBindTexture(GL_TEXTURE_2D, texture);
			int w, h;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
			width = w;
			height = h;
		}
		void setTexture(std::string path) { 
			this->texture = loadTexture(path.c_str()); 
			glBindTexture(GL_TEXTURE_2D, texture);
			int w, h;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
			width = w;
			height = h;
		}
		void setDimensions(glm::vec2 dimensions) {
			width = dimensions.x;
			height = dimensions.y;
		}
		void fitWidth(double width) {
			if (texture == 0)
				setDimensions(glm::vec2(width));
			glm::vec2 dim = originalSize();
			double ratio = dim.y / dim.x;
			this->width = width;
			height = width * ratio;
		}
		void fitHeight(double height) {
			if (texture == 0)
				setDimensions(glm::vec2(height));
			glm::vec2 dim = originalSize();
			double ratio = dim.x / dim.y;
			this->height = height;
			width = height * ratio;
		}
		void fit(glm::vec2 bounds, glm::vec4 margin = glm::vec4(0)) {
			if (texture == 0)
				setDimensions(bounds);
			glm::vec2 dim = originalSize();

			if ((bounds.x - margin.x - margin.z) / dim.x <= (bounds.y - margin.y - margin.w) / dim.y) {
				fitWidth(bounds.x - margin.x - margin.z);
				y += (bounds.y - margin.y - margin.w - height) / 2.0 + margin.y;
				x += margin.x;
			}
			else {
				fitHeight(bounds.y - margin.y - margin.w);
				x += (bounds.x - width - margin.x - margin.z) / 2.0 + margin.x;
				y += margin.y;
			}
		}
		void fill(glm::vec2 bounds, glm::vec4 margin = glm::vec4(0)) {
			if (texture == 0)
				setDimensions(bounds);
			glm::vec2 dim = originalSize();

			if ((bounds.x - margin.x - margin.z) / dim.x <= (bounds.y - margin.y - margin.w) / dim.y) {
				fitHeight(bounds.y - margin.y - margin.w);
				x += (bounds.x - width - margin.x - margin.z) / 2.0 + margin.x;
				y += margin.y;
			}
			else {
				fitWidth(bounds.x - margin.x - margin.z);
				y += (bounds.y - margin.y - margin.w - height) / 2.0 + margin.y;
				x += margin.x;
			}
		}
		void fitParent(glm::vec4 margin = glm::vec4(0)) {
			if (!parent) {
				if (!window)
					return;
				fit(glm::vec2(window->width, window->height), margin);
			}
			else
				fit(glm::vec2(parent->width, parent->height), margin);
		}
		void fillParent(glm::vec4 margin = glm::vec4(0)) {
			if (!parent) {
				if (!window)
					return;
				fill(glm::vec2(window->width, window->height), margin);
			}
			else
				fill(glm::vec2(parent->width, parent->height), margin);
		}
		glm::vec2 originalSize() {
			if (texture == 0)
				return glm::vec2(0);
			glBindTexture(GL_TEXTURE_2D, texture);
			int w, h;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
			return glm::vec2(w, h);
		}
	};

	extern window* current_window, * base_window;
	extern std::map<GLFWwindow*, window*> win_ref;
	extern std::map<std::string, std::tuple<unsigned int, unsigned int, GLenum, GLboolean, GLsizei, std::vector<unsigned int>>> vao_definitions;
	extern void init();
	extern void update();
	extern unsigned int addVAODefinition(std::string name, unsigned int vbo, unsigned int ebo, GLenum vbo_data_type, GLboolean normalize_vbo_data, GLsizei size_of_vbo_data_in_bytes, std::vector<unsigned int> positions_and_offsets);
	extern window* create_window(const char* = "Window", int = 1280, int = 720);
}
#endif