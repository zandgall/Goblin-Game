#ifndef GL_HELPER
#define GL_HELPER
#pragma once
#ifdef _WIN32
#include <WinSock2.h>
#include <windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#else
#include <sys/socket.h>
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <glad/glad.h>
#include <KHR/khrplatform.h>
#define GLM_FORCE_SWIZZLE
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <stb_image.h>
#include <functional>

#include <AL/alc.h>

#include <ft2build.h>
#include FT_FREETYPE_H

extern GLFWwindow* WINDOW;
extern int WINDOW_WIDTH, WINDOW_HEIGHT;

extern ALCdevice* device;
extern ALCcontext* context;

extern bool BIND_BUFFERS_ON_CREATE;

extern double pmouseX, pmouseY, pmouseScroll;
extern double mouseX, mouseY, mouseScroll;
extern bool pmouseLeft, pmouseRight, pmouseMiddle;
extern bool mouseLeft, mouseRight, mouseMiddle;

extern bool* keys, * pKeys;

struct Z_Glyph {
	unsigned int textureID;
	glm::ivec2 Size;
	glm::ivec2 Bearing;
	unsigned int Advance;
};

extern FT_Library freetype;
extern std::map<std::string, std::map<uint64_t, Z_Glyph>> fonts;
extern std::map<std::string, int> fontsizes;
extern std::map<std::string, FT_Face> font_faces;

#ifdef USE_CACHE
extern std::map<std::string, unsigned int> CACHE;
#endif

/// <summary>
/// Initiates OpenGL through GLFW
/// </summary>
/// <param name="verMajor">Major part of version number</param>
/// <param name="verMinor">Minor part of version number</param>
/// <param name="name">Name of the window to be created</param>
/// <param name="windowWidth">Width of window to be created</param>
/// <param name="windowHeight">Height of window to be created</param>
/// <returns>0 if success, -1 if error</returns>
int START_OPEN_GL(char verMajor, char verMinor, const char* name, int windowWidth, int windowHeight);

/// <summary>
/// Passes a uniform float to the current Shader Program
/// </summary>
/// <param name="name">The name/identifier of the uniform variable</param>
/// <param name="value">The value to give it</param>
void uniFloat(const GLchar* name, const float value);
/// <summary>
/// Passes a uniform integer to the current Shader Program
/// </summary>
/// <param name="name">The name/identifier of the uniform variable</param>
/// <param name="value">The value to give it</param>
void uniInt(const GLchar* name, const int value);
/// <summary>
/// Passes a uniform unsigned integer to the current Shader Program
/// </summary>
/// <param name="name">The name/identifier of the uniform variable</param>
/// <param name="value">The value to give it</param>
void uniUInt(const GLchar* name, const unsigned int value);
/// <summary>
/// Passes a uniform double to the current Shader Program
/// </summary>
/// <param name="name">The name/identifier of the uniform variable</param>
/// <param name="value">The value to give it</param>
void uniDouble(const GLchar* name, const double value);
/// <summary>
/// Passes a uniform boolean to the current Shader Program
/// </summary>
/// <param name="name">The name/identifier of the uniform variable</param>
/// <param name="value">The value to give it</param>
void uniBool(const GLchar* name, const bool value);

template <typename T, int L>
void uniVec(const GLchar* name, const glm::vec<L, T, glm::packed_highp> data) {
	GLint shader = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &shader);
	int location = glGetUniformLocation(shader, name);
	unsigned char switchExpr = (typeid(T) == typeid(unsigned short) || typeid(T) == typeid(unsigned int)) + 2 * (typeid(T) == typeid(float)) + 3 * (typeid(T) == typeid(double));
	if (switchExpr == 0 && !(typeid(T) == typeid(int) || typeid(T) == typeid(bool) || typeid(T) == typeid(short)))
		switchExpr = -1; // is unsupported or unknown
	switch (switchExpr) {
	case 0:
		switch (L) {
		case 1:
			glUniform1iv(location, 1, (const GLint*)glm::value_ptr(data));
			break;
		case 2:
			glUniform2iv(location, 1, (const GLint*)glm::value_ptr(data));
			break;
		case 3:
			glUniform3iv(location, 1, (const GLint*)glm::value_ptr(data));
			break;
		case 4:
			glUniform4iv(location, 1, (const GLint*)glm::value_ptr(data));
			break;
		default:
			glUniform1iv(location, L, (const GLint*)glm::value_ptr(data));
		}
		break;
	case 1:
		switch (L) {
		case 1:
			glUniform1uiv(location, 1, (const GLuint*)glm::value_ptr(data));
			break;
		case 2:
			glUniform2uiv(location, 1, (const GLuint*)glm::value_ptr(data));
			break;
		case 3:
			glUniform3uiv(location, 1, (const GLuint*)glm::value_ptr(data));
			break;
		case 4:
			glUniform4uiv(location, 1, (const GLuint*)glm::value_ptr(data));
			break;
		default:
			glUniform1uiv(location, L, (const GLuint*)glm::value_ptr(data));
		}
		break;
	case 2:
		switch (L) {
		case 1:
			glUniform1fv(location, 1, (const GLfloat*)glm::value_ptr(data));
			break;
		case 2:
			glUniform2fv(location, 1, (const GLfloat*)glm::value_ptr(data));
			break;
		case 3:
			glUniform3fv(location, 1, (const GLfloat*)glm::value_ptr(data));
			break;
		case 4:
			glUniform4fv(location, 1, (const GLfloat*)glm::value_ptr(data));
			break;
		default:
			glUniform1fv(location, L, (const GLfloat*)glm::value_ptr(data));
		}
		break;
	case 3:
		switch (L) {
		case 1:
			glUniform1dv(location, 1, (const GLdouble*)glm::value_ptr(data));
			break;
		case 2:
			glUniform2dv(location, 1, (const GLdouble*)glm::value_ptr(data));
			break;
		case 3:
			glUniform3dv(location, 1, (const GLdouble*)glm::value_ptr(data));
			break;
		case 4:
			glUniform4dv(location, 1, (const GLdouble*)glm::value_ptr(data));
			break;
		default:
			glUniform1dv(location, L, (const GLdouble*)glm::value_ptr(data));
		}
		break;
	default:
		std::cerr << "uniVec: Unsupported or unknown glm vector type! \"" << typeid(T).name() << "\"";
	}
}

template <typename T, int C> // Only allows square matrices (so far)
void uniMat(const GLchar* name, const glm::mat<C, C, T> data, const GLboolean transpose = GL_FALSE) {
	GLint shader = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &shader);
	int location = glGetUniformLocation(shader, name);
	switch (C) {
	case 2:
		if (typeid(T) == typeid(float))
			glUniformMatrix2fv(location, 1, transpose, (const GLfloat*)glm::value_ptr(data));
		else
			glUniformMatrix2dv(location, 1, transpose, (const GLdouble*)glm::value_ptr(data));
		break;
	case 3:
		if (typeid(T) == typeid(float))
			glUniformMatrix3fv(location, 1, transpose, (const GLfloat*)glm::value_ptr(data));
		else
			glUniformMatrix3dv(location, 1, transpose, (const GLdouble*)glm::value_ptr(data));
		break;
	case 4:
		if (typeid(T) == typeid(float))
			glUniformMatrix4fv(location, 1, transpose, (const GLfloat*)glm::value_ptr(data));
		else
			glUniformMatrix4dv(location, 1, transpose, (const GLdouble*)glm::value_ptr(data));
		break;
	}
}

void createVBO(GLenum* vbo, void* vboData, const unsigned int dataLength = 0, const GLenum usage = GL_STATIC_DRAW);
void createVAO(GLenum* vao);
void createEBO(GLenum* ebo, void* eboData, const unsigned int dataLength = 0, const GLenum usage = GL_STATIC_DRAW);
//void defineAttribute(const unsigned int index, const GLenum type, const GLboolean normalized, )
// Returns a translation matrix
glm::mat4 translation(glm::vec3 pos);
// Returns a rotation matrix off of a given matrix
glm::mat4 rotation(glm::mat4 mat, glm::vec3 rot);
// Returns a rotation matrix off of [1]
glm::mat4 rotation(glm::vec3 rot);
// Returns a scalar matrix
glm::mat4 scaled(glm::vec3 scaling);
// Returns a positioned, rotated, and possibly scaled matrix off of given matrix, or identity matrix if unspecified
glm::mat4 form(glm::vec3 pos, glm::vec3 scale = glm::vec3(1), glm::vec3 rot = glm::vec3(0), glm::mat4 mat = glm::mat4(1));
// Returns a transform function for a specified rectangle shape
glm::mat4 rect(float x, float y, float w, float h, glm::vec3 rot = glm::vec3(0), glm::mat4 mat = glm::mat4(1));

unsigned int compileShader(unsigned int type, const std::string& source);

unsigned int loadShader(const char* filepath);

unsigned int loadTexture(const char* filepath, int magFilter = GL_LINEAR, int minFilter = GL_LINEAR_MIPMAP_LINEAR, int wrapS = GL_CLAMP_TO_EDGE, int wrapT = GL_CLAMP_TO_EDGE);

void loadFont(const char* filepath, const char* fontName, int size, uint64_t chars = 256);

void drawString(std::string string, const char* font, float scale, float x, float y, void (*callEveryChar)() = NULL);

glm::vec3 measureString(std::string, const char*);
#endif
