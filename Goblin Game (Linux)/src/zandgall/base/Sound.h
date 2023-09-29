#pragma once
#include <AL/al.h>
#include <AL/alc.h>
class Sound {
public:
	ALuint source, buffer;
	Sound();
	// Only handles OGG VORBIS at the moment
	Sound(const char* filepath);
	void loadOgg(const char* filepath);
	void play();
	void pause();
	void stop();
	void kill();
};

