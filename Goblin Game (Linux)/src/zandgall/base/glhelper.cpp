 #include "glhelper.h"
#include "handler.h"
#include "../appwork/appwork.h"

GLFWwindow* WINDOW;
int WINDOW_WIDTH, WINDOW_HEIGHT;

bool BIND_BUFFERS_ON_CREATE = true;

ALCdevice* device;
ALCcontext* context;

double pmouseX, pmouseY, pmouseScroll;
double mouseX, mouseY, mouseScroll;
bool pmouseLeft, pmouseRight, pmouseMiddle;
bool mouseLeft, mouseRight, mouseMiddle;

bool* keys = new bool[GLFW_KEY_LAST];
bool* pKeys = new bool[GLFW_KEY_LAST];

FT_Library freetype;
std::map<std::string, std::map<uint64_t, Z_Glyph >> fonts = std::map < std::string, std::map < uint64_t, Z_Glyph >>();
std::map<std::string, int> fontsizes = std::map<std::string, int>();
std::map<std::string, FT_Face> font_faces = std::map<std::string, FT_Face> ();
#ifdef USE_CACHE
std::map<std::string, unsigned int> CACHE = std::map<std::string, unsigned int>();
#endif
void framebuffer_size_callback(GLFWwindow* wnd, int width, int height) {
	WINDOW_WIDTH = width;
	WINDOW_HEIGHT = height;
}

void keypress_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	keys[key] = action == GLFW_PRESS || action == GLFW_REPEAT;
}

void mouse_scroll_call(GLFWwindow* window, double x, double y) {
	pmouseScroll = mouseScroll;
	mouseScroll = y;
}

void mouse_move_call(GLFWwindow* window, double x, double y) {
	pmouseX = mouseX;
	pmouseY = mouseY;
	mouseX = x;
	mouseY = y;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		pmouseLeft = mouseLeft;
		mouseLeft = action == GLFW_PRESS;
		break;
	case GLFW_MOUSE_BUTTON_RIGHT:
		pmouseRight = mouseRight;
		mouseRight = action == GLFW_PRESS;
		break;
	case GLFW_MOUSE_BUTTON_MIDDLE:
		pmouseMiddle = mouseMiddle;
		mouseMiddle = action == GLFW_PRESS;
		break;
	}
}

void APIENTRY glDebugOutput(GLenum source,
	GLenum type,
	unsigned int id,
	GLenum severity,
	GLsizei length,
	const char* message,
	const void* userParam)
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

/// <summary>
/// Initiates OpenGL through GLFW
/// </summary>
/// <param name="verMajor">Major part of version number</param>
/// <param name="verMinor">Minor part of version number</param>
/// <param name="name">Name of the window to be created</param>
/// <param name="windowWidth">Width of window to be created</param>
/// <param name="windowHeight">Height of window to be created</param>
/// <returns>0 if success, -1 if error</returns>
int START_OPEN_GL(char verMajor, char verMinor, const char* name, int windowWidth, int windowHeight) {
	// Initiate
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, verMajor);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, verMinor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	// Create a window and use it
	WINDOW = glfwCreateWindow(windowWidth, windowHeight, name, NULL, NULL);
	WINDOW_WIDTH = windowWidth;
	WINDOW_HEIGHT = windowHeight;
	if (WINDOW == NULL) {
		//std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(WINDOW);

	// Callbacks
	glfwSetFramebufferSizeCallback(WINDOW, framebuffer_size_callback);
	glfwSetCursorPosCallback(WINDOW, mouse_move_call);
	glfwSetMouseButtonCallback(WINDOW, mouse_button_callback);
	glfwSetKeyCallback(WINDOW, keypress_callback);
	glfwSetScrollCallback(WINDOW, mouse_scroll_call);

	if (FT_Init_FreeType(&freetype)) {
		std::cout << "Could not init FreeType Library" << std::endl;
		return -1;
	}

	// Load OpenGL extensions via glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Glad loading failed!" << std::endl;
		return -1;
	}

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(glDebugOutput, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

	for (int i = 0; i < GLFW_KEY_LAST; i++) {
		keys[i] = false;
		pKeys[i] = false;
	}

	device = alcOpenDevice(NULL);
	if (device) {
		context = alcCreateContext(device, NULL);
		if (context) {
			alcMakeContextCurrent(context);
			std::cout << "Successfully loaded OpenAL" << std::endl;
		}
		else {
			alcCloseDevice(device);
		}
	}

	return 0;
}

void uniFloat(const GLchar* name, const float value) {
	GLint shader = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &shader);
	int location = glGetUniformLocation(shader, name);
	glUniform1f(location, value);
}
void uniInt(const GLchar* name, const int value) {
	GLint shader = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &shader);
	int location = glGetUniformLocation(shader, name);
	glUniform1i(location, value);
}
void uniUInt(const GLchar* name, const unsigned int value) {
	GLint shader = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &shader);
	int location = glGetUniformLocation(shader, name);
	glUniform1ui(location, value);
}
void uniDouble(const GLchar* name, const double value) {
	GLint shader = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &shader);
	int location = glGetUniformLocation(shader, name);
	glUniform1d(location, value);
}
void uniBool(const GLchar* name, const bool value) { // Actually just uniInt in disguise..
	uniInt(name, value);
}

// Returns a translation matrix
glm::mat4 translation(glm::vec3 pos) {
	return glm::translate(glm::mat4(1.0), pos);
}
// Returns a rotation matrix off of a given matrix
glm::mat4 rotation(glm::mat4 mat, glm::vec3 rot) {
	mat = glm::rotate(mat, rot.x, glm::vec3(1.0, 0.0, 0.0));
	mat = glm::rotate(mat, rot.y, glm::vec3(0.0, 1.0, 0.0));
	mat = glm::rotate(mat, rot.z, glm::vec3(0.0, 0.0, 1.0));
	return mat;
}
// Returns a rotation matrix off of [1]
glm::mat4 rotation(glm::vec3 rot) {
	glm::mat4 out = glm::mat4(1.0);
	out = glm::rotate(out, rot.x, glm::vec3(1.0, 0.0, 0.0));
	out = glm::rotate(out, rot.y, glm::vec3(0.0, 1.0, 0.0));
	out = glm::rotate(out, rot.z, glm::vec3(0.0, 0.0, 1.0));
	return out;
}
// Returns a scalar matrix
glm::mat4 scaled(glm::vec3 scaling) {
	glm::mat4 out = glm::mat4(1.0);
	out = glm::scale(out, scaling);
	return out;
}
// Returns a positioned, rotated, and possibly scaled matrix off of given matrix, or [1]
glm::mat4 form(glm::vec3 pos, glm::vec3 scale, glm::vec3 rot, glm::mat4 mat) {
	return glm::scale(rotation(glm::translate(mat, pos), rot), scale);
}
// Returns a transform function for a specified rectangle shape
glm::mat4 rect(float x, float y, float w, float h, glm::vec3 rot, glm::mat4 mat) {
	return form(glm::vec3(x + w / 2.f, y + h / 2.f, 0), glm::vec3(w / 2.f, h / 2.f, 0), rot, mat);
}

unsigned int compileShader(unsigned int type, const std::string& source) {
	unsigned int id = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);

	if (result == GL_FALSE) {
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);

		char* message = (char*)malloc(length * sizeof(char));

		glGetShaderInfoLog(id, length, &length, message);

		std::string line;
		std::istringstream stream(source);
		int lineNum = 0;
		while (std::getline(stream, line)) {
			std::cout << ++lineNum << ": " << line << std::endl;
		}

		std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << std::endl;
		std::cout << message << std::endl;
		glDeleteShader(id);
		return 0;
	}

	return id;
}

unsigned int loadShader(const char* filepath) {
#ifdef USE_CACHE
	if (CACHE.find(filepath) != CACHE.end())
		return CACHE[filepath];
#endif
	auto pre = glfwGetCurrentContext();
	glfwMakeContextCurrent(app::base_window->glfw_window);
	std::ifstream stream(filepath);

	enum class ShaderType {
		NONE = -1, VERTEX = 0, FRAGMENT = 1
	};

	std::string line;
	std::stringstream ss[2];
	ShaderType type = ShaderType::NONE;
	int lineNum = 1;
	while (getline(stream, line)) {
		if (line.find("#shader") != std::string::npos) {
			if (line.find("vertex") != std::string::npos)
				type = ShaderType::VERTEX;
			else if (line.find("fragment") != std::string::npos)
				type = ShaderType::FRAGMENT;
			lineNum = 1;
		}
		else {
			//std::cout << lineNum << " : " << line << std::endl;
			lineNum++;
			ss[(size_t)type] << line << '\n';
		}
	}

	stream.clear();
	stream.close();

	unsigned int program = glCreateProgram();

	unsigned int vs = 0;
	vs = compileShader(GL_VERTEX_SHADER, ss[0].str());
	unsigned int fs = 0;
	fs = compileShader(GL_FRAGMENT_SHADER, ss[1].str());

	ss[0].clear();
	ss[1].clear();
	line.clear();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glValidateProgram(program);

	glDeleteShader(vs);
	glDeleteShader(fs);
#ifdef USE_CACHE
	CACHE.insert(std::make_pair(filepath, program));
#endif
	glfwMakeContextCurrent(pre);
	return program;
}

unsigned int loadTexture(const char* filepath, int magFilter, int minFilter, int wrapS, int wrapT) {
#ifdef USE_CACHE
	if (CACHE.find(filepath) != CACHE.end())
		return CACHE[filepath];
#endif
	auto pre = glfwGetCurrentContext();
	glfwMakeContextCurrent(app::base_window->glfw_window);
	GLuint texture;						// Have empty number,
	glGenTextures(1, &texture);					// Gen texture id into that number,
	glBindTexture(GL_TEXTURE_2D, texture);		// Bind it

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);			// Repeat/clip/etc image horizontally
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);			// Repeat/clip/etc image vertically
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);	// Set interpolation at minimum
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);	// Set interpolation at maximum

	int width, height, nrChannels;					// Have empty width, height and channel numbers
	std::string string = std::string(filepath);		// Load the chars into a string
	stbi_set_flip_vertically_on_load(false);

	unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, STBI_rgb_alpha); // Load the texture and get width, height, and channel number
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);	// Buffer data into the texture
		glGenerateMipmap(GL_TEXTURE_2D);														// Generate mipmap based on interpolation
	}
	else {
		std::cout << "Failed to load texture " << filepath << std::endl;	// Scream if needed
		return 0;
	}
	stbi_image_free(data);					// Empty data,
#ifdef USE_CACHE
	CACHE.insert(std::make_pair(filepath, texture));
#endif
	glfwMakeContextCurrent(pre);
	return texture;							// and return the texture
}

void createVBO(GLenum* vbo, void* vboData, const unsigned int dataLength, GLenum usage) {
	glGenBuffers(1, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, *vbo);
	glBufferData(GL_ARRAY_BUFFER, dataLength == 0 ? sizeof(vboData) : dataLength, vboData, usage);
}
void createVAO(GLenum* vao) {
	glGenVertexArrays(1, vao);
	if (BIND_BUFFERS_ON_CREATE)
		glBindVertexArray(*vao);
}
void createEBO(GLenum* ebo, void* eboData, const unsigned int dataLength, GLenum usage) {
	glGenBuffers(1, ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, dataLength == 0 ? sizeof(eboData) : dataLength, eboData, usage);
}

void loadFont(const char* filepath, const char* fontName, int size, uint64_t chars) {
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	FT_Face face;
	if (FT_New_Face(freetype, filepath, 0, &face)) {
		std::cout << "Could not load font " << filepath << " (" << fontName << ")" << std::endl;
		return;
	}
	FT_Set_Pixel_Sizes(face, 0, size);

	font_faces.insert(std::make_pair(fontName, face));
	fonts.insert(std::make_pair(fontName, std::map<uint64_t, Z_Glyph>()));
	fontsizes.insert(std::make_pair(fontName, size));

	for (uint64_t c = 0; c < chars; c++) {
		if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
			std::cout << "Couldn't load char #" << c << " (" << fontName << ")" << std::endl;
			continue;
		}

		unsigned int texture = 0;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
		// set texture options
		GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// now store character for later use
		Z_Glyph glyph = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};
		fonts[fontName].insert(std::make_pair(c, glyph));

	}
	//FT_Done_Face(face); We keep the face loaded for future data use!

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void drawString(std::string string, const char* font, float scale, float x, float y, void (*callEveryChar)()) {
	glBindVertexArray(app::win_ref[glfwGetCurrentContext()]->globjects["Square VAO"]);

	float xPos = x;
	//float yPos = y;
	for (std::string::const_iterator c = string.begin(); c != string.end(); c++) {
		Z_Glyph glyph = fonts[std::string(font)][*c];

		uniMat("transform", rect(xPos + glyph.Bearing.x*scale, y - glyph.Bearing.y * scale, glyph.Size.x * scale, glyph.Size.y * scale));
		uniBool("useTex", true);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, glyph.textureID);
		//callEveryChar();
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		xPos += (glyph.Advance >> 6) * scale;
	}
}

glm::vec3 measureString(std::string string, const char* font) {
	float x = 0;
	float y = 0;
	for (std::string::const_iterator c = string.begin(); c != string.end(); c++) {
		Z_Glyph glyph = fonts[std::string(font)][*c];
		x += (glyph.Advance >> 6);
		y = std::max<float>(y, glyph.Size.y);
	}

	return glm::vec3(x, y, fontsizes[font]);
}