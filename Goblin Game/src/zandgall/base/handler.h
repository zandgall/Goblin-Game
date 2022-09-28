#ifndef HANDLER_H
#define HANDLER_H

#include <GLFW/glfw3.h>
#include <vector>
#include <map>
#include <string>
/// <summary>
/// A list of unspecified pointers, accessible from anywhere using strings to identify each pointer. Easy functions to use are _addPointer, _setPointer, and _getPointer.
/// </summary>
extern std::map<std::string, void*> GLOBAL_POINTERS;
/// <summary>
/// A list of unspecified GLuint OpenGL objects
/// </summary>
extern std::map<std::string, GLuint> GLOBAL_GL_OBJECTS;

/// <summary>
/// A list of OpenGL Shader programs
/// </summary>
extern std::map<std::string, GLuint> GL_SHADERS;
extern void _addPointer(std::string name, void* pointer);
extern void _setPointer(std::string name, void* pointer);
extern void* _getPointer(std::string name);
extern void _addGLObject(std::string name, GLuint object);
extern void _setGLObject(std::string name, GLuint object);
extern unsigned int _getGLObject(std::string name);
extern void _addGLShader(std::string name, GLuint object);
extern void _setGLShader(std::string name, GLuint object);
extern unsigned int _getGLShader(std::string name);
#endif