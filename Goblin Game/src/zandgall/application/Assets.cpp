#include "Assets.h"
#include "../base/glhelper.h"
unsigned int goblin_sheet, run, still, color_mask;
unsigned int worm_sheet;
void loadAssets() {
	goblin_sheet = loadTexture("res/textures/goblin/goblin.png", GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST);
	run = loadTexture("res/textures/goblin/run.png", GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST, GL_REPEAT, GL_REPEAT);
	still = loadTexture("res/textures/goblin/still.png", GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST, GL_REPEAT, GL_REPEAT);
	color_mask = loadTexture("res/textures/goblin/mask.png", GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST);
	worm_sheet = loadTexture("res/textures/worm.png", GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST, GL_REPEAT, GL_REPEAT);
}