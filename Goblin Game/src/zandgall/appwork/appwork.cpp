#include "../base/glhelper.h"
#include "../base/handler.h"
#include "../base/Sound.h"
#include "appwork.h"
//Low and behold.. (ONLY IN SOURCE!)
using namespace std;
using namespace app;

std::map<GLFWwindow*, window*> app::win_ref = std::map<GLFWwindow*, window*>();
window* app::current_window = nullptr, * app::base_window = nullptr;
map<string, tuple<unsigned int, unsigned int, GLenum, GLboolean, GLsizei, vector<unsigned int>>> app::vao_definitions = map<string, tuple<unsigned int, unsigned int, GLenum, GLboolean, GLsizei, vector<unsigned int>>>();
void _framebuffer_size_callback(GLFWwindow* wnd, int width, int height) {
	win_ref[wnd]->width = width;
	win_ref[wnd]->height = height;
}

void _keypress_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	win_ref[window]->keys[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
	for (auto a : app::win_ref[window]->key_typed)
		a(window, key, scancode, action, mods);
}

void _mouse_scroll_call(GLFWwindow* window, double x, double y) {
	//win_ref[window]->pmouseScroll = win_ref[window]->mouseScroll;
	win_ref[window]->mouseScroll = y;
}

void _mouse_move_call(GLFWwindow* window, double x, double y) {
	//win_ref[window]->pmouseX = win_ref[window]->mouseX;
	//win_ref[window]->pmouseY = win_ref[window]->mouseY;
	win_ref[window]->mouseX = x;
	win_ref[window]->mouseY = y;
}

void _mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		//win_ref[window]->pmouseLeft = win_ref[window]->mouseLeft;
		win_ref[window]->mouseLeft = action == GLFW_PRESS;
		break;
	case GLFW_MOUSE_BUTTON_RIGHT:
		//win_ref[window]->pmouseRight = win_ref[window]->mouseRight;
		win_ref[window]->mouseRight = action == GLFW_PRESS;
		break;
	case GLFW_MOUSE_BUTTON_MIDDLE:
		//win_ref[window]->pmouseMiddle = win_ref[window]->mouseMiddle;
		win_ref[window]->mouseMiddle = action == GLFW_PRESS;
		break;
	}
}

void _char_call(GLFWwindow* window, unsigned int codepoint) {
	std::cout << codepoint << std::endl;
	for (auto a : app::win_ref[window]->char_funcs)
		a(window, codepoint);
}

void APIENTRY _glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length,	const char* message, const void* userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}

void app::init() {
	START_OPEN_GL(3, 3, "window", 100, 100);
	base_window = new window(WINDOW);
	win_ref.emplace(make_pair(WINDOW, base_window));
}

void app::update() {
	for (auto a = win_ref.begin(); a != win_ref.end(); a++) {
		if (a->first == base_window->glfw_window)
			continue;
		if (glfwWindowShouldClose(a->first)) {
			glfwDestroyWindow(a->first);
			delete a->second;
			a->second->glfw_window = nullptr;
			a->second = nullptr;
			win_ref.erase(a);
			return;
		}
		a->second->default_update();
		a->second->default_draw();

		glfwSwapBuffers(a->first);
		std::copy(a->second->keys, &a->second->keys[GLFW_KEY_LAST - 1], a->second->pKeys);
		a->second->pmouseLeft = a->second->mouseLeft;
		a->second->pmouseRight = a->second->mouseRight;
		a->second->pmouseMiddle = a->second->mouseMiddle;
		a->second->pmouseScroll = a->second->mouseScroll;
		a->second->pmouseX = a->second->mouseX;
		a->second->pmouseY = a->second->mouseY;
		a->second->mouseScroll = 0;
	}
	glfwPollEvents();
}

void create_vao_from_definition(app::window* win, string name) {
	auto& def = app::vao_definitions[name];
	glfwMakeContextCurrent(win->glfw_window);
	win->addGLObject(name, 0);
	glGenVertexArrays(1, &win->globjects[name]);
	glBindVertexArray(win->globjects[name]);
	glBindBuffer(GL_ARRAY_BUFFER, get<0>(def));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, get<1>(def));

	unsigned int size = get<5>(def).size();
	unsigned int total_elements = 0;
	for (int i = 0; i < size; i++)
		total_elements += get<5>(def)[i];
	unsigned int single_size = get<4>(def) / total_elements;
	unsigned int offset = 0;
	for (int i = 0; i < size; i++) {
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, get<5>(def)[i], get<2>(def), get<3>(def), get<4>(def), (GLvoid*)offset);
		offset += get<5>(def)[i] * single_size;
	}
	glBindVertexArray(0);
}

unsigned int app::addVAODefinition(string name, unsigned int vbo, unsigned int ebo, GLenum type, GLboolean nrmlized, GLsizei size, vector<unsigned int> attribp) {
	//vao_definitions.emplace(make_pair(name, make_pair(vbo, ebo)));
	vao_definitions.emplace(make_pair(name, make_tuple(vbo, ebo, type, nrmlized, size, attribp)));
	for (auto windows = win_ref.begin(); windows != win_ref.end(); windows++) {
		create_vao_from_definition(windows->second, name);
	}
	if (current_window)
		return current_window->globjects[name];
	return base_window->globjects[name];
}

app::window* app::create_window(const char* name, int width, int height) {
	glfwWindowHint(GLFW_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
	GLFWwindow* glfwwin = glfwCreateWindow(width, height, name, NULL, base_window->glfw_window);
	glfwSetFramebufferSizeCallback(glfwwin, _framebuffer_size_callback);
	glfwSetCursorPosCallback(glfwwin, _mouse_move_call);
	glfwSetMouseButtonCallback(glfwwin, _mouse_button_callback);
	glfwSetKeyCallback(glfwwin, _keypress_callback);
	glfwSetScrollCallback(glfwwin, _mouse_scroll_call);
	glfwSetCharCallback(glfwwin, _char_call);
	//Keep in mind for text input

	win_ref.insert(std::make_pair(glfwwin, new window(glfwwin)));
	win_ref[glfwwin]->width = width;
	win_ref[glfwwin]->height = height;

	for (int i = 0; i < GLFW_KEY_LAST; i++) {
		win_ref[glfwwin]->keys[i] = 0;
		win_ref[glfwwin]->pKeys[i] = 0;
	}

	glfwMakeContextCurrent(glfwwin);

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(_glDebugOutput, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

	for (auto a : vao_definitions) {
		create_vao_from_definition(win_ref[glfwwin], a.first);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return win_ref[glfwwin];
}

// Classes and function params

void app::component::default_update() {
	if (this->window->mouseLeft && !this->window->pmouseLeft && this->window->mouseX >= bounds().x && this->window->mouseX <= bounds().x + width && this->window->mouseY >= bounds().y && this->window->mouseY <= bounds().y + height)
		this->window->selected_component = this;
	run_all(update, this);
	for (auto a : children) {
		a->parent = this;
		a->window = window;
		a->default_update();
	}
}

void text_draw_function(component* component) {
	app::text* text = (app::text*)(component);
	uniVec("col", text->text_color);
	float ascension = float(font_faces[text->font]->ascender) * (float(text->font_size) / float(font_faces[text->font]->height));
	drawString(text->string, text->font.c_str(), float(text->font_size) / fontsizes[text->font], text->bounds().x, text->bounds().y+ascension);
}
app::text::text() {
	this->background_color = glm::vec4(0);
	this->draw.push_back(&text_draw_function);
}

void button_update_function(component* component) {
	app::button* button = (app::button*)(component);
	auto mouseX = button->window->mouseX;
	auto mouseY = button->window->mouseY;
	auto mouseLeft = button->window->mouseLeft;
	auto pmouseLeft = button->window->pmouseLeft;
	if (!button->toggle)
		button->value = false;
	if (mouseX >= button->bounds().x && mouseX <= button->bounds().x + button->width && mouseY >= button->bounds().y && mouseY <= button->bounds().y + button->height) {
		if (!button->hovered)
			run_all(button->on_hover, button);
		button->hovered = true;
		run_all(button->while_hovered, button);

		if (mouseLeft) {
			if (!pmouseLeft)
				run_all(button->on_click, button);
			run_all(button->while_mouse_held, button);
			button->held = true;
		}
		else if (pmouseLeft) {
			run_all(button->on_release, button);
			if (button->toggle) {
				button->value = !button->value;
				if (button->value)
					run_all(button->on_toggled_on, button);
				else run_all(button->on_toggled_off, button);
			}
			else button->value = true; 
			button->held = false;
		}
	}
	else {
		if (button->hovered)
			run_all(button->on_not_hover, button);
		button->hovered = false;
		button->held = false;
		run_all(button->while_not_hovered, button);
	}

	if (button->toggle)
		if (button->value)
			run_all(button->while_toggled_on, button);
		else run_all(button->while_toggled_off, button);

	if (!mouseLeft || !button->hovered)
		run_all(button->while_released, button);
}
app::button::button() {
	this->update.push_back(&button_update_function);
}

void def_on_click(app::button* button) { button->background_color = glm::vec4(0.4, 0.4, 0.4, 1); }
void def_on_release(app::button* button) { button->background_color = glm::vec4(0.6, 0.6, 0.6, 1); }
void def_on_hovered(app::button* button) { button->background_color = glm::vec4(0.6, 0.6, 0.6, 1); }
void def_on_not_hovered(app::button* button) { button->background_color = glm::vec4(0.5, 0.5, 0.5, 1); }

void app::button::setDefaultStyle() {
	background_color = glm::vec4(0.5, 0.5, 0.5, 1);
	on_click.push_back(&def_on_click);
	on_release.push_back(&def_on_release);
	on_hover.push_back(&def_on_hovered);
	on_not_hover.push_back(&def_on_not_hovered);
	shader = _getGLShader("round shaded rect shader");
	uniforms << new nbt::floattag("rounding", min(25.0, min(width/2, height/2)));
	uniforms << new nbt::floattag("shadingAmount", 0.5);
}

void slider_update_function(component* component) {
	slider* slider = (app::slider*)component;

	auto mouseX = slider->window->mouseX;
	auto mouseY = slider->window->mouseY;
	auto mouseLeft = slider->window->mouseLeft;

	if (mouseX >= slider->bounds().x && mouseX <= slider->bounds().x + slider->width && mouseY >= slider->bounds().y && mouseY <= slider->bounds().y + slider->height) {
		if (slider->step > 0)
			slider->value += slider->window->mouseScroll * slider->step;
		else {
			if (slider->vertical)
				slider->value += slider->window->mouseScroll * 2.0 / slider->height;
			else slider->value += slider->window->mouseScroll * 2.0 / slider->width;
		}
	}
	if (slider->window->selected_component && (slider->window->selected_component == slider || slider->window->selected_component->isChildOf(slider)) && mouseLeft) {
		if (slider->vertical)
			slider->value = ((mouseY - slider->bounds().y) / slider->height) * (slider->maximum - slider->minimum) + slider->minimum;
		else slider->value = ((mouseX - slider->bounds().x - slider->height * 0.5) / (slider->width - slider->height)) * (slider->maximum - slider->minimum) + slider->minimum;
	}

	slider->value = max(min(slider->value, slider->maximum), slider->minimum);
	if (slider->step > 0 && slider->value != slider->maximum && slider->value != slider->minimum)
		slider->value = round(slider->value / slider->step);
	if (slider->vertical) {
		slider->thumb->y = slider->value * (slider->height - slider->width);
		slider->thumb->x = 0;
		slider->thumb->width = slider->width;
		slider->thumb->height = slider->width;
	}
	else {
		slider->thumb->x = slider->value * (slider->width - slider->height);
		slider->thumb->y = 0;
		slider->thumb->height = slider->height;
		slider->thumb->width = slider->height;
	}
	slider->thumb->window = slider->window;
}
app::slider::slider() {
	this->update.push_back(&slider_update_function);
	thumb = new component();
	addChild(thumb);
}

void app::slider::setDefaultStyle() {
	background_color = glm::vec4(0.3, 0.3, 0.3, 1);
	thumb->background_color = glm::vec4(0.5, 0.5, 0.5, 1);
	shader = _getGLShader("round shaded rect shader");
	uniforms.add(new nbt::floattag("rounding", 25));
	uniforms.add(new nbt::floattag("shadingAmount", 0.5));
	thumb->shader = _getGLShader("round bezeled rect shader");
	thumb->uniforms.add(new nbt::floattag("rounding", 25));
	thumb->uniforms.add(new nbt::floattag("shadingAmount", 0.5));
	if (vertical) {
		thumb->width = this->width;
		thumb->height = this->width;
	}
	else {
		thumb->width = this->height;
		thumb->height = this->height;
	}
}

void draggable_interact_held_function(button* interact) {
	draggable* draggable = (app::draggable*)interact->parent;
	if (!interact->window->pmouseLeft) {
		draggable->offset = glm::vec2(draggable->window->mouseX - draggable->x, draggable->window->mouseY - draggable->y);
	}
	draggable->x = interact->window->mouseX - draggable->offset.x;
	draggable->y = interact->window->mouseY - draggable->offset.y;
}
void draggable_interact_release_function(button* interact) {
	draggable* draggable = (app::draggable*)interact->parent;
	if (draggable->sticky) {
		draggable->x = draggable->origin.x;
		draggable->y = draggable->origin.y;
	}
	draggable->origin = glm::vec2(draggable->x, draggable->y);
}

void draggable_update_function(component* component) {
	draggable* draggable = (app::draggable*)component;

	if(draggable->origin == glm::vec2(0))
		draggable->origin = glm::vec2(draggable->x, draggable->y);
	if (draggable->window->selected_component && (draggable->window->selected_component == draggable || draggable->window->selected_component->isChildOf(draggable))) {
		if (!draggable->window->pmouseLeft) {
			draggable->offset = glm::vec2(draggable->window->mouseX - draggable->x, draggable->window->mouseY - draggable->y);
		}
		draggable->x = draggable->window->mouseX - draggable->offset.x;
		draggable->y = draggable->window->mouseY - draggable->offset.y;
	}

	if (!draggable->window->mouseLeft) {
		if (draggable->sticky) {
			draggable->x = draggable->origin.x;
			draggable->y = draggable->origin.y;
		}
		draggable->origin = glm::vec2(draggable->x, draggable->y);
	}
}

app::draggable::draggable() {
	update.push_back(&draggable_update_function);
}

void image_draw_function(component* component) {
	image* image = (app::image*)component;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, image->texture);
	if (image->background_color == glm::vec4(1, 1, 1, 0))
		uniVec("col", glm::vec4(1));
	uniBool("useTex", true);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

app::image::image() {
	texture = 0;
	background_color = glm::vec4(1, 1, 1, 0);
	draw.push_back(&image_draw_function);
}

app::textinput* current_text_input = nullptr;

const std::string word_break = "!@#$%^&*()-=_+\\|[]{}\'\"';:,.<>/?`~ ";
void text_input_key_function(GLFWwindow* window, int key, int scan, int action, int mods) {
	if (current_text_input == nullptr)
		return;
	if (current_text_input != win_ref[window]->selected_component)
		return;
	if (action == GLFW_RELEASE)
		return;
	std::string& text = current_text_input->display->string;
	int& pos = current_text_input->marker;
	pos = std::max<int>(pos, 0);
	int& sel = current_text_input->select_marker;
	sel = std::max<int>(sel, 0);
	switch(key) {
	case GLFW_KEY_BACKSPACE:
		if (pos != sel) {
			glm::vec3 metr = measureString(text.substr(min(pos, sel), abs(pos - sel)), current_text_input->display->font.c_str());
			metr /= metr.z;
			metr *= current_text_input->display->font_size;
			current_text_input->display->x += metr.x;
			text.erase(min(pos, sel), abs(pos - sel));
			pos = min(pos, sel);
			sel = pos;
		}
		else if (pos > 0) {
			glm::vec3 metr = measureString(text.substr(pos-1, 1), current_text_input->display->font.c_str());
			metr /= metr.z;
			metr *= current_text_input->display->font_size;
			current_text_input->display->x += metr.x;
			text.erase(pos - 1, 1);
			--pos;
		}
		break;
	case GLFW_KEY_DELETE:
		if (pos != sel) {
			glm::vec3 metr = measureString(text.substr(min(pos, sel), abs(pos - sel)), current_text_input->display->font.c_str());
			metr /= metr.z;
			metr *= current_text_input->display->font_size;
			current_text_input->display->x += metr.x;
			text.erase(min(pos, sel), abs(pos - sel));
			pos = min(pos, sel);
			sel = pos;
		}
		else if (pos < text.size()) {
			glm::vec3 metr = measureString(text.substr(pos, 1), current_text_input->display->font.c_str());
			metr /= metr.z;
			metr *= current_text_input->display->font_size;
			current_text_input->display->x += metr.x;
			text.erase(pos, 1);
		}
		break;
	case GLFW_KEY_HOME:
	case GLFW_KEY_UP:
	case GLFW_KEY_PAGE_UP:
		pos = 0;
		break;
	case GLFW_KEY_END:
	case GLFW_KEY_DOWN:
	case GLFW_KEY_PAGE_DOWN:
		pos = text.size();
		break;
	case GLFW_KEY_LEFT:
		if (mods & GLFW_MOD_CONTROL) {
			while (--pos > 0 && word_break.find(text[pos - 1]) == std::string::npos);
			if (pos < 0)
				pos = 0;
		}
		else if (pos > 0)
			--pos;
		break;
	case GLFW_KEY_RIGHT:
		if (mods & GLFW_MOD_CONTROL)
			while (pos < text.size() && word_break.find(text[++pos]) == std::string::npos);
		else if (pos < text.size())
			++pos;
		break;
	case GLFW_KEY_ENTER:
	case GLFW_KEY_ESCAPE:
		current_text_input = nullptr;
		win_ref[window]->selected_component = nullptr;
		break;
	case GLFW_KEY_V:
		if (mods & GLFW_MOD_CONTROL) {
			if (pos != sel) {
				text.erase(min(pos, sel), abs(pos - sel));
				pos = min(pos, sel);
				sel = pos;
			}
			text.insert(pos, glfwGetClipboardString(window));
			pos += strlen(glfwGetClipboardString(window));
		}
		else return;
	default:
		return;
		// Other text is handled in char callback
	};
	if (!(mods & GLFW_MOD_SHIFT))
		sel = pos;
	if (current_text_input == nullptr)
		return;
	glm::vec3 metr = measureString(text.substr(0, pos), current_text_input->display->font.c_str());
	metr /= metr.z;
	metr *= current_text_input->display->font_size;
	if (current_text_input->display->x + metr.x + fonts[current_text_input->display->font]['|'].Bearing.x > current_text_input->width)
		current_text_input->display->x = std::min<float>(current_text_input->width - metr.x - fonts[current_text_input->display->font]['|'].Bearing.x, 0);
	if (current_text_input->display->x + metr.x < 0)
		current_text_input->display->x = std::min<float>(-metr.x, 0);
	current_text_input->display->x = std::min<float>(current_text_input->display->x, 0);
}

void text_input_char_function(GLFWwindow* window, unsigned int codepoint) {
	if (current_text_input == nullptr)
		return;
	if (current_text_input != win_ref[window]->selected_component)
		return;
	std::string& text = current_text_input->display->string;
	int& pos = current_text_input->marker;
	int& sel = current_text_input->select_marker;
	char c = (char)codepoint; // Yeah.. just ignore the utf32 and stick to ASCII/least sig byte
	if (pos != sel) {
		text.erase(min(pos, sel), abs(pos - sel));
		pos = min(pos, sel);
		sel = pos;
	}
	text.insert(text.begin() + pos, c);
	++pos;
	glm::vec3 metr = measureString(text.substr(0, pos), current_text_input->display->font.c_str());
	metr /= metr.z;
	metr *= current_text_input->display->font_size;
	//metr.x -= fonts[current_text_input->display->font]['|'].Bearing.x * 0.5;
	if (current_text_input->display->x + metr.x + fonts[current_text_input->display->font]['|'].Bearing.x > current_text_input->width)
		current_text_input->display->x = std::min<float>(current_text_input->width-metr.x - fonts[current_text_input->display->font]['|'].Bearing.x, 0);
	sel = pos;

	metr = measureString(text.substr(0, pos), current_text_input->display->font.c_str());
	metr /= metr.z;
	metr *= current_text_input->display->font_size;
	if (current_text_input->display->x + metr.x + fonts[current_text_input->display->font]['|'].Bearing.x > current_text_input->width)
		current_text_input->display->x = std::min<float>(current_text_input->width - metr.x - fonts[current_text_input->display->font]['|'].Bearing.x, 0);
	if (current_text_input->display->x + metr.x < 0)
		current_text_input->display->x = std::min<float>(-metr.x, 0);
	current_text_input->display->x = std::min<float>(current_text_input->display->x, 0);
}

void text_input_update(component* component) {
	app::textinput* textinput = (app::textinput*)component;
	if (textinput->window->selected_component && (textinput == textinput->window->selected_component || textinput->window->selected_component->isChildOf(textinput))) {
		current_text_input = textinput;
		auto glyph = fonts[textinput->display->font]['|'];
		if (!textinput->window->pmouseLeft && textinput->window->mouseLeft && !(textinput->window->keys[GLFW_KEY_LEFT_SHIFT] || textinput->window->keys[GLFW_KEY_RIGHT_SHIFT])) { // Just clicked
			if (textinput->display->string.size() == 0)
				return;

			for (int i = 0; i < textinput->display->string.size()+1; i++) {
				glm::vec3 metr = measureString(textinput->display->string.substr(0, std::min<int>(i+1, textinput->display->string.size())), textinput->display->font.c_str());
				metr /= metr.z;
				metr *= textinput->display->font_size;
				if (i < textinput->display->string.size())
					metr.x -= fonts[textinput->display->font][textinput->display->string[i]].Size.x + fonts[textinput->display->font][textinput->display->string[i]].Bearing.x * 0.5;
				else metr.x += fonts[textinput->display->font][textinput->display->string[i - 1]].Bearing.x*0.5 - fonts[textinput->display->font][textinput->display->string[i-1]].Size.x * 0.5;
				if (metr.x + textinput->display->bounds().x > textinput->window->mouseX) {
					textinput->select_marker = i-1;
					textinput->marker = i - 1;
					return;
				} 
			}
			textinput->marker = textinput->display->string.size();
			textinput->select_marker = textinput->display->string.size();
			 
		}
		else if (textinput->window->mouseLeft) {
			if (textinput->display->string.size() == 0)
				return;
			for (int i = 0; i < textinput->display->string.size()+1; i++) {
				glm::vec3 metr = measureString(textinput->display->string.substr(0, std::min<int>(i + 1, textinput->display->string.size())), textinput->display->font.c_str());
				metr /= metr.z;
				metr *= textinput->display->font_size;
				if (i < textinput->display->string.size())
					metr.x -= fonts[textinput->display->font][textinput->display->string[i]].Size.x + fonts[textinput->display->font][textinput->display->string[i]].Bearing.x * 0.5;
				else metr.x += fonts[textinput->display->font][textinput->display->string[i - 1]].Bearing.x * 0.5 - fonts[textinput->display->font][textinput->display->string[i - 1]].Size.x * 0.5;
				if (metr.x + textinput->display->bounds().x > textinput->window->mouseX) {
					textinput->marker = i - 1;
					return;
				}
			}
			textinput->marker = textinput->display->string.size();

		}
	}
}

void text_input_draw(component* component) {
	if(component->window->selected_component != component)
		return;
	app::textinput* textinput = (app::textinput*)component;
	glEnable(GL_SCISSOR_TEST);
	glScissor(textinput->bounds().x, textinput->window->height - textinput->bounds().y - textinput->height, textinput->width, textinput->height);
	auto glyph = fonts[textinput->display->font]['|'];
	glm::vec3 metr = measureString(textinput->display->string.substr(0, textinput->marker), textinput->display->font.c_str());
	metr /= metr.z;
	metr *= textinput->display->font_size;
	metr.x -= glyph.Bearing.x*0.5;
	if (sin(glfwGetTime() * 3.14159265 * 2) > 0) {
		float ascension = float(font_faces[textinput->display->font]->ascender) * (float(textinput->display->font_size) / float(font_faces[textinput->display->font]->height));
		uniVec("col", textinput->display->text_color);
		drawString("|", textinput->display->font.c_str(), float(textinput->display->font_size) / fontsizes[textinput->display->font], textinput->display->bounds().x + metr.x - glyph.Size.x * 0.5 + (glyph.Bearing.x * float(textinput->display->font_size) / fontsizes[textinput->display->font]), textinput->display->bounds().y + ascension);
	}
	glm::vec3 metr2 = measureString(textinput->display->string.substr(0, textinput->select_marker), textinput->display->font.c_str());
	metr2 /= metr2.z;
	metr2 *= textinput->display->font_size;
	//metr2.x -= fonts[textinput->display->font]['|'].Bearing.x * 0.5;
	metr.x += glyph.Bearing.x * 0.5;
	uniMat("transform", rect(textinput->display->bounds().x + min(metr.x, metr2.x), textinput->bounds().y, abs(metr.x - metr2.x), textinput->height));
	uniVec("col", glm::vec4(0, 0, 1, 0.5));
	uniBool("useTex", false);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glDisable(GL_SCISSOR_TEST);
}

void component_parent_scissor(component* component) {
	glEnable(GL_SCISSOR_TEST);
	if (component->parent) {
		glScissor(component->parent->bounds().x, component->window->height-component->parent->bounds().y-component->parent->height, component->parent->width, component->parent->height);
	}
}
void component_disable_scissor(component* component) {
	glDisable(GL_SCISSOR_TEST);
}

app::textinput::textinput(app::window* window) {
	this->window = window;
	display = new text(0, 0);
	display->draw.insert(display->draw.begin(), &component_parent_scissor);
	display->draw.push_back(&component_disable_scissor);
	window->key_typed.push_back(&text_input_key_function);
	window->char_funcs.push_back(&text_input_char_function);
	update.push_back(&text_input_update);
	draw.push_back(&text_input_draw);
	addChild(display);
}