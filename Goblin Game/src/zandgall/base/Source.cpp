/*

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
                  PROJECT TEMPLATE v0.0.7
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is an C++ OpenGL Project template
Project specific code files go into a seperate folder (or just filter if desired),
renamed to whatever the project is, of course.
Any framework can be kept in the "base" folder, these can
later be added to the Project Template for use in future
projects.

Cheers! Happy Coding!
    ~ Zandgall
*/

#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define NBT_SHORTHAND
#define NBT_INCLUDE
#define NBT_IGNORE_MUTF
#define NBT_COMPILE
#include <nbt/nbt.hpp>
#define NBT_GZNBT_INCLUDE
#include <loadgz/gznbt.h>

#define GLM_FORCE_SWIZZLE
#include "glhelper.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include <stb_easy_font.h>
#include "handler.h"
#include <thread>
#include "../application/App.h"
#define APPWORK_IMPLEMENT
#include "../appwork/appwork.h" 

#include <stdlib.h>
#include <stdio.h>

int main(const char* args) {
    //START_OPEN_GL(4, 6, "OpenGL Template", 1280, 720);
    app::init(); 
    glfwMakeContextCurrent(app::base_window->glfw_window);
    _addGLShader("default shader", loadShader("res/shaders/shader.shader"));
    _addGLShader("goblin shader", loadShader("res/shaders/goblin.shader"));
    _addGLShader("round shader", loadShader("res/shaders/round.shader"));
    _addGLShader("round rect shader", loadShader("res/shaders/round rect.shader"));
    _addGLShader("round shaded rect shader", loadShader("res/shaders/round shaded rect.shader"));
    _addGLShader("round bezeled rect shader", loadShader("res/shaders/round bezeled rect.shader"));
    glUseProgram(_getGLShader("default shader"));
    loadFont("res/fonts/robotomono.ttf", "roboto", 64);

    
    // Square VAO definitions 
    float vertices[] = {
      1.0f,  1.0f, 0.0f,  // top right
      1.0f, -1.0f, 0.0f,  // bottom right
     -1.0f, -1.0f, 0.0f,  // bottom left
     -1.0f,  1.0f, 0.0f   // top left 
    };
    unsigned int indices[] = {
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };
    _addGLObject("Square VBO", 0);
    _addGLObject("Square EBO", 0);
    createVBO(&GLOBAL_GL_OBJECTS["Square VBO"], vertices, sizeof(vertices));
    createEBO(&GLOBAL_GL_OBJECTS["Square EBO"], indices, sizeof(indices));
    //_addGLObject("Square VAO", 0);
    //createVAO(&GLOBAL_GL_OBJECTS["Square VAO"]);
    //glEnableVertexAttribArray(0);
    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); 
    app::addVAODefinition("Square VAO", _getGLObject("Square VBO"), _getGLObject("Square EBO"), GL_FLOAT, GL_FALSE, 3 * sizeof(float), std::vector<unsigned int>{3});

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    using fps60 = std::chrono::duration<int64_t, std::ratio<1, 60>>;
    using clock = std::chrono::high_resolution_clock;
    auto currentTime = clock::now() - fps60{1}, frame_next = clock::now() + fps60{1};
    double delta = 0, fps_last = glfwGetTime(), frame_start = glfwGetTime();
    int frames = 0, fps = 0;


    App app = App();
    // Main game loop
    while (!glfwWindowShouldClose(WINDOW)) {
        currentTime = clock::now();
        frame_next += fps60{1};

        double currentGLFWTime = glfwGetTime();
        delta = currentGLFWTime - frame_start;
        frame_start = currentGLFWTime;
        //std::cout << "Delta: " << delta << std::endl;
        if (delta < 0.01)
            frame_next = currentTime + fps60{ 1 };
        frames++;
        if (currentGLFWTime - fps_last >= 1) {
            //std::cout << frames << " fps" << std::endl;
            fps = frames;
            frames = 0;
            fps_last = currentGLFWTime;
        }
        delta = (1 / 60.0);
        
        //Tick
        //if (app.window->keys[GLFW_KEY_SPACE]) {
            app.tick(delta);
        //}
        //Render
        app.render();

        app::update();
        //frame_next += fps60{ 5 };
        std::this_thread::sleep_until(frame_next);
    }

    glfwTerminate();
    return 0;
}