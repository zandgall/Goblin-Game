#include "Sound.h"
#include <al.h>
#include <alc.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#include <vector>
#include <iostream>

Sound::Sound() {
	buffer = 0, source = 0;
}

Sound::Sound(const char* filepath) {
	alGenSources(1, &source);
	loadOgg(filepath);
	alSourcei(source, AL_BUFFER, buffer);
}

void Sound::kill() {
	alDeleteBuffers(1, &buffer);
	alDeleteSources(1, &source);
}

void Sound::loadOgg(const char* filepath) {
	FILE* file = 0;
	fopen_s(&file, filepath, "rb");
	if (file == 0) {
		std::cout << "Error: Unable to open ogg file \"" << filepath << "\"" << std::endl;
		return;
	}
	alGenBuffers(1, &buffer);
	OggVorbis_File vorbis_file;
	ov_open_callbacks(file, &vorbis_file, NULL, 0, OV_CALLBACKS_NOCLOSE);
	vorbis_info* info = ov_info(&vorbis_file, -1);
	
	ALenum format = info->channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	size_t data_len = ov_pcm_total(&vorbis_file, -1) * info->channels * 2;
	int16_t *pcmout = (int16_t*)malloc(data_len);

	for (size_t size = 0, offset = 0, sel = 0;
		(size = ov_read(&vorbis_file, (char*)pcmout + offset, 4096, 0, 2, 1, (int*)&sel)) != 0; offset += size) {
		if (size < 0) {
			std::cout << "Error: Bad OGG file! \"" << filepath << "\"" << std::endl;
			free(pcmout);
			fclose(file);
			ov_clear(&vorbis_file);
			return;
		}
	}

	alBufferData(buffer, format, pcmout, data_len, info->rate);
	free(pcmout);
	fclose(file);
	ov_clear(&vorbis_file);
}

void Sound::play() {
	alSourcePlay(source);
}

void Sound::pause() {
	alSourcePause(source);
}

void Sound::stop() {
	alSourceStop(source);
}