#include "handler.h"
std::map<std::string, void*> GLOBAL_POINTERS = std::map<std::string, void*>();
std::map<std::string, GLuint> GLOBAL_GL_OBJECTS = std::map<std::string, GLuint>();
std::map<std::string, GLuint> GL_SHADERS = std::map<std::string, GLuint>();
void _addPointer(std::string name, void* pointer) { GLOBAL_POINTERS.emplace(std::make_pair(name, pointer)); }
void _setPointer(std::string name, void* pointer) { GLOBAL_POINTERS[name] = pointer; }
void* _getPointer(std::string name) { return GLOBAL_POINTERS[name]; }
void _addGLObject(std::string name, GLuint object) { GLOBAL_GL_OBJECTS.emplace(std::make_pair(name, object)); }
void _setGLObject(std::string name, GLuint object) { GLOBAL_GL_OBJECTS[name] = object; }
unsigned int _getGLObject(std::string name) { return GLOBAL_GL_OBJECTS[name]; }
void _addGLShader(std::string name, GLuint object) { GL_SHADERS.emplace(std::make_pair(name, object)); }
void _setGLShader(std::string name, GLuint object) { GL_SHADERS[name] = object; }
unsigned int _getGLShader(std::string name) { return GL_SHADERS[name]; }